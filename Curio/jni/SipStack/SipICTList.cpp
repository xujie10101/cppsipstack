/* 
 * Copyright (C) 2012 Yee Young Han <websearch@naver.com> (http://blog.naver.com/websearch)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include "SipICTList.h"
#include "SipStack.h"
#include "SipDeleteQueue.h"

CSipICTList::CSipICTList()
{
}

CSipICTList::~CSipICTList()
{
	DeleteAll();
}

/**
 * @brief Invite Client Transaction List �� SIP �޽����� �߰��Ѵ�.
 * @param pclsMessage SIP �޽��� ���� ����ü
 * @returns �����ϸ� true �� �����ϰ� �����ϸ� false �� �����Ѵ�.
 */
bool CSipICTList::Insert( CSipMessage * pclsMessage )
{
	std::string strKey;
	INVITE_TRANSACTION_MAP::iterator	itMap;
	bool	bRes = false;

	if( pclsMessage == NULL ) return false;
	if( GetKey( pclsMessage, strKey ) == false ) return false;

	if( pclsMessage->IsRequest() )
	{
		m_clsMutex.acquire();
		itMap = m_clsMap.find( strKey );
		if( itMap == m_clsMap.end() )
		{
			if( pclsMessage->IsMethod( "ACK" ) == false )
			{
				CSipInviteTransaction * psttTransaction = new CSipInviteTransaction();
				if( psttTransaction )
				{
					psttTransaction->m_pclsRequest = pclsMessage;
					gettimeofday( &psttTransaction->m_sttStartTime, NULL );

					m_clsMap.insert( INVITE_TRANSACTION_MAP::value_type( strKey, psttTransaction ) );
					bRes = true;
				}
			}
		}
		else if( pclsMessage->IsMethod( "ACK" ) )
		{
			itMap->second->m_pclsAck = pclsMessage;
			gettimeofday( &itMap->second->m_sttStopTime, NULL );
			bRes = true;
		}

		if( bRes )
		{
			pclsMessage->MakePacket();
		}

		m_clsMutex.release();
	}
	else
	{
		m_clsMutex.acquire();
		itMap = m_clsMap.find( strKey );
		if( itMap != m_clsMap.end() )
		{
			if( itMap->second->m_pclsResponse )
			{
				if( itMap->second->m_iStatusCode != pclsMessage->m_iStatusCode )
				{
					gclsSipDeleteQueue.Insert( itMap->second->m_pclsResponse );
					itMap->second->m_pclsResponse = pclsMessage;
					itMap->second->m_iStatusCode = pclsMessage->m_iStatusCode;
					bRes = true;
				}
			}
			else
			{
				itMap->second->m_pclsResponse = pclsMessage;
				itMap->second->m_iStatusCode = pclsMessage->m_iStatusCode;
				bRes = true;
			}

			// 180/183 ���� �޽����� ������ ��, 200 OK �� �������� �� �� ��� ICT ���� �����ϱ� ���� ���
			// 200 OK �� �����ϰ� ACK �� �������� ���� ��� ICT ���� �����ϱ� ���� ���
			gettimeofday( &itMap->second->m_sttRingTime, NULL );

			if( itMap->second->m_pclsAck )
			{
				m_pclsSipStack->Send( itMap->second->m_pclsAck );
			}
		}
		m_clsMutex.release();
	}

	return bRes;
}

/**
 * @brief Invite Client Transaction List ���� �������� �׸��� �������ϰ� ������ �׸��� �����ϰ� Timeout �� �׸��� Timeout ó���Ѵ�.
 * @param psttTime ������ �ð�
 */
void CSipICTList::Execute( struct timeval * psttTime )
{
	if( m_clsMap.size() == 0 ) return;

	INVITE_TRANSACTION_MAP::iterator	itMap, itNext;
	OSIP_MESSAGE_LIST	clsResponseList;

	m_clsMutex.acquire();
	for( itMap = m_clsMap.begin(); itMap != m_clsMap.end(); ++itMap )
	{
LOOP_START:
		if( itMap->second->m_sttStopTime.tv_sec > 0 )
		{
			if( DiffTimeval( &itMap->second->m_sttStopTime, psttTime ) >= 32000 )
			{
DELETE_TRANSACTION:
				itNext = itMap;
				++itNext;

				delete itMap->second;
				m_clsMap.erase( itMap );

				if( itNext == m_clsMap.end() ) break;
				itMap = itNext;
				goto LOOP_START;
			}
		}
		else if( itMap->second->m_iStatusCode == 0 )
		{
			if( DiffTimeval( &itMap->second->m_sttStartTime, psttTime ) >= m_arrICTReSendTime[itMap->second->m_iReSendCount] )
			{
				++itMap->second->m_iReSendCount;
				if( itMap->second->m_iReSendCount == MAX_ICT_RESEND_COUNT )
				{
					if( itMap->second->m_pclsResponse == NULL )
					{
						CSipMessage * psttResponse = itMap->second->m_pclsRequest->CreateResponse( SIP_REQUEST_TIME_OUT );
						if( psttResponse )
						{
							clsResponseList.push_back( psttResponse );
						}
					}

					goto DELETE_TRANSACTION;
				}
				else
				{
					m_pclsSipStack->Send( itMap->second->m_pclsRequest );
				}
			}
		}
		else if( itMap->second->m_sttRingTime.tv_sec > 0 )
		{
			if( DiffTimeval( &itMap->second->m_sttRingTime, psttTime ) >= SIP_RING_TIMEOUT )
			{
				goto DELETE_TRANSACTION;
			}
		}
	}
	m_clsMutex.release();

	for( OSIP_MESSAGE_LIST::iterator itList = clsResponseList.begin(); itList != clsResponseList.end(); ++itList )
	{
		m_pclsSipStack->RecvResponse( m_pclsSipStack->m_clsSetup.m_iUdpThreadCount, *itList );
		delete *itList;
	}

	clsResponseList.clear();
}

/**
 * @brief Invite Client Transaction List �� ��� �׸��� �����Ѵ�.
 */
void CSipICTList::DeleteAll( )
{
	INVITE_TRANSACTION_MAP::iterator	itMap;

	m_clsMutex.acquire();
	for( itMap = m_clsMap.begin(); itMap != m_clsMap.end(); ++itMap )
	{
		delete itMap->second;
	}

	m_clsMap.clear();
	m_clsMutex.release();
}

/**
 * @brief Invite Client Transaction List �� ũ�⸦ �����Ѵ�.
 * @returns Invite Client Transaction List �� ũ�⸦ �����Ѵ�.
 */
int CSipICTList::GetSize( )
{
	return m_clsMap.size();
}

void CSipICTList::GetString( std::string & strBuf )
{
	INVITE_TRANSACTION_MAP::iterator	itMap;

	strBuf.clear();

	m_clsMutex.acquire();
	for( itMap = m_clsMap.begin(); itMap != m_clsMap.end(); ++itMap )
	{
		strBuf.append( itMap->first );
		strBuf.append( "\n" );
	}
	m_clsMutex.release();
}