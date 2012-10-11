<%@ page language="java" contentType="text/html; charset=EUC-KR" pageEncoding="EUC-KR"%>
<%@ page import="java.sql.*" %>
<%@ include file="DbInfo.jsp" %>
<%
	Connection m_clsDbConn = null;
	
	try
	{
		Class.forName("com.mysql.jdbc.Driver");
		
		m_clsDbConn = DriverManager.getConnection( "jdbc:mysql://localhost/ksipserver", m_strDbUserId, m_strDbPassWord );
	}
	catch( Exception e )
	{
		e.printStackTrace( );
	}
%>
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=EUC-KR">
<title>SipUserList</title>
</head>
<body>
<b>SIP User List</b>
<br><br>
<table border=1 cellspacing=0 cellpadding=7 >
	<tr>
		<td width="100" align="center"><b>UserId</b></td>
		<td width="40" align="center"><b>DND</b></td>
		<td width="100" align="center"><b>CallForward</b></td>
	</tr>
<%
	if( m_clsDbConn != null )
	{
		Statement clsStmt = m_clsDbConn.createStatement( );
		String strSQL = "SELECT Id, DND, CallForward FROM sipuser";
		
		ResultSet clsRS = clsStmt.executeQuery( strSQL );
		
		while( clsRS.next() )
		{
%>
	<tr>
		<td><%= clsRS.getString( 1 ) %></td>
		<td align="center"><%= clsRS.getString( 2 ) %></td>
		<td><%= clsRS.getString( 3 ) %></td>
	</tr>
<%			
		}
	}
%>

</table>
</body>
</html>
<%
	try
	{
		if( m_clsDbConn != null ) m_clsDbConn.close();
	}
	catch( Exception e )
	{
		e.printStackTrace( );
	}
%>