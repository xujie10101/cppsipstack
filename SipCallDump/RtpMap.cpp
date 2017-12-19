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

#include "RtpMap.h"
#include "SipCallMap.h"
#include "SipCallDumpSetup.h"
#include "TimeUtility.h"
#include "MemoryDebug.h"

CRtpMap gclsRtpMap;

CRtpInfo::CRtpInfo() : m_iRecvTime(0)
{
}

CRtpMap::CRtpMap()
{
}

CRtpMap::~CRtpMap()
{
}

bool CRtpMap::Insert( struct pcap_pkthdr * psttHeader, const u_char * pszData, Ip4Header * psttIpHeader, UdpHeader * psttUdpHeader )
{
	RTP_MAP::iterator itMap;
	std::string strKey, strCallId;
	bool bRes = false;

	GetKey( psttIpHeader->daddr, psttUdpHeader->dport, strKey );

	m_clsMutex.acquire();
	itMap = m_clsMap.find( strKey );
	if( itMap != m_clsMap.end() )
	{
		time( &itMap->second.m_iRecvTime );
		strCallId = itMap->second.m_strCallId;
		bRes = true;
	}
	m_clsMutex.release();

	if( bRes )
	{
		gclsCallMap.Insert( strCallId.c_str(), psttHeader, pszData );
	}

	return bRes;
}

bool CRtpMap::Insert( const char * pszIp, int iPort, const char * pszCallId )
{
	RTP_MAP::iterator itMap;
	std::string strKey;
	bool bRes = false;

	GetKey( pszIp, iPort, strKey );

	m_clsMutex.acquire();
	itMap = m_clsMap.find( strKey );
	if( itMap == m_clsMap.end() )
	{
		CRtpInfo clsInfo;

		clsInfo.m_strCallId = pszCallId;
		time( &clsInfo.m_iRecvTime );

		m_clsMap.insert( RTP_MAP::value_type( strKey, clsInfo ) );
		bRes = true;
	}
	m_clsMutex.release();

	return bRes;
}

bool CRtpMap::Delete( const char * pszIp, int iPort )
{
	RTP_MAP::iterator itMap;
	std::string strKey;
	bool bRes = false;

	GetKey( pszIp, iPort, strKey );

	m_clsMutex.acquire();
	itMap = m_clsMap.find( strKey );
	if( itMap != m_clsMap.end() )
	{
		m_clsMap.erase( itMap );
		bRes = true;
	}
	m_clsMutex.release();

	return bRes;
}

void CRtpMap::SelectTimeout( STRING_LIST & clsCallIdList )
{
	RTP_MAP::iterator itMap;
	time_t iTime;

	time( &iTime );

	m_clsMutex.acquire();
	for( itMap = m_clsMap.begin(); itMap != m_clsMap.end(); ++itMap )
	{
		if( ( iTime - itMap->second.m_iRecvTime ) >= gclsSetup.m_iRtpRecvTimeout )
		{
			clsCallIdList.push_back( itMap->second.m_strCallId );
		}
	}
	m_clsMutex.release();
}

/**
 * @ingroup SipCallDump
 * @brief �ڷᱸ�� ����� Ű�� �����Ѵ�.
 * @param pszIp		IP �ּ�
 * @param iPort		��Ʈ ��ȣ
 * @param strKey	Ű
 */
void CRtpMap::GetKey( const char * pszIp, int iPort, std::string & strKey )
{
	uint32_t iIp = inet_addr( pszIp );
	uint16_t sPort = htons( iPort );

	GetKey( iIp, sPort, strKey );
}

/**
 * @ingroup SipCallDump
 * @brief �ڷᱸ�� ����� Ű�� �����Ѵ�.
 * @param iIp			IP �ּ�
 * @param sPort		��Ʈ ��ȣ
 * @param strKey	Ű
 */
void CRtpMap::GetKey( uint32_t iIp, uint16_t sPort, std::string & strKey )
{
	char	szKey[21];

	snprintf( szKey, sizeof(szKey), "%x:%x", iIp, sPort );
	strKey = szKey;
}