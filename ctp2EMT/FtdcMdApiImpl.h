#if !defined(_FTDCMDAPIIMPL_H)
#define _FTDCMDAPIIMPL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "emt_quote_api.h"
#include "ThostFtdcMdApi.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <thread>

using namespace EMT::API;

///API�ӿ�ʵ��
class CFtdcMdApiImpl : public CThostFtdcMdApi, public QuoteSpi
{
public:	
	///���캯��
	CFtdcMdApiImpl(const char* pszFlowPath);

	void OnTime(const boost::system::error_code& err);
		
	///��ȡAPI�İ汾��Ϣ
	///@retrun ��ȡ���İ汾��
	//const char *GetApiVersion(){return 0;};

	///ɾ���ӿڶ�����
	///@remark ����ʹ�ñ��ӿڶ���ʱ,���øú���ɾ���ӿڶ���
	virtual void Release();
	
	///��ʼ��
	///@remark ��ʼ�����л���,ֻ�е��ú�,�ӿڲſ�ʼ����
	virtual void Init();
	
	///�ȴ��ӿ��߳̽�������
	///@return �߳��˳�����
	virtual int Join();
	
	///��ȡ��ǰ������
	///@retrun ��ȡ���Ľ�����
	///@remark ֻ�е�¼�ɹ���,���ܵõ���ȷ�Ľ�����
	virtual const char *GetTradingDay();
	
	///ע��ǰ�û������ַ
	///@param pszFrontAddress��ǰ�û������ַ��
	///@remark �����ַ�ĸ�ʽΪ����protocol://ipaddress:port�����磺��tcp://127.0.0.1:17001���� 
	///@remark ��tcp��������Э�飬��127.0.0.1�������������ַ����17001������������˿ںš�
	virtual void RegisterFront(char *pszFrontAddress);
	
	///ע�����ַ����������ַ
	///@param pszNsAddress�����ַ����������ַ��
	///@remark �����ַ�ĸ�ʽΪ����protocol://ipaddress:port�����磺��tcp://127.0.0.1:12001���� 
	///@remark ��tcp��������Э�飬��127.0.0.1�������������ַ����12001������������˿ںš�
	///@remark RegisterFront������RegisterNameServer
	virtual void RegisterNameServer(char *pszNsAddress);

	///ע��ص��ӿ�
	///@param pSpi �����Իص��ӿ����ʵ��
	virtual void RegisterSpi(CThostFtdcMdSpi *pSpi);

	///�û���¼����
	virtual int ReqUserLogin(CThostFtdcReqUserLoginField *pReqUserLogin, int nRequestID);
	virtual void HandleReqUserLogin(CThostFtdcReqUserLoginField& ReqUserLogin, int nRequestID);

	///�û��˳�����
	virtual int ReqUserLogout(CThostFtdcUserLogoutField *pUserLogout, int nRequestID);
	virtual void HandleReqUserLogout(CThostFtdcUserLogoutField& UserLogout, int nRequestID);

	///�������顣
	///@param ppInstrumentID ��ԼID  
	///@param nCount Ҫ����/�˶�����ĺ�Լ����
	///@remark 
	virtual int SubscribeMarketData(char* ppInstrumentID[], int nCount);

	///�˶����顣
	///@param ppInstrumentID ��ԼID  
	///@param nCount Ҫ����/�˶�����ĺ�Լ����
	///@remark 
	virtual int UnSubscribeMarketData(char* ppInstrumentID[], int nCount);

	///ע�����ַ������û���Ϣ
	///@param pFensUserInfo���û���Ϣ��
	virtual void RegisterFensUserInfo(CThostFtdcFensUserInfoField * pFensUserInfo){return;};

	///����ѯ�ۡ�
	///@param ppInstrumentID ��ԼID  
	///@param nCount Ҫ����/�˶�����ĺ�Լ����
	///@remark 
	virtual int SubscribeForQuoteRsp(char* ppInstrumentID[], int nCount) { return -3; }

	///�˶�ѯ�ۡ�
	///@param ppInstrumentID ��ԼID  
	///@param nCount Ҫ����/�˶�����ĺ�Լ����
	///@remark 
	virtual int UnSubscribeForQuoteRsp(char* ppInstrumentID[], int nCount) { return -3; }

#if defined(V6_3_19) || defined(V6_5_1) || defined(V6_6_1_P1)
	///�����ѯ�鲥��Լ
	virtual int ReqQryMulticastInstrument(CThostFtdcQryMulticastInstrumentField *pQryMulticastInstrument, int nRequestID) { return -3; }
#endif

	virtual void OnDisconnected(int reason);

	/*���鶩��Ӧ��*/
	virtual void OnSubMarketData(EMTQuoteStaticInfo* ticker, EMTRspInfoStruct* error_info, bool is_last);

	/*�����˶�Ӧ��*/
	virtual void OnUnSubMarketData(EMTQuoteStaticInfo* ticker, EMTRspInfoStruct* error_info, bool is_last);

	/*����֪ͨ*/
	virtual void OnDepthMarketData(EMTMarketDataStruct* market_data, int64_t bid1_qty[], int32_t bid1_count, int32_t max_bid1_count, int64_t ask1_qty[], int32_t ask1_count, int32_t max_ask1_count);

public:
	EMT::API::QuoteApi*m_pUserApi;
	CThostFtdcMdSpi *m_pSpi;
	TThostFtdcDateType TradingDay;
	boost::asio::io_service m_io_service;
	std::thread* m_pthread;
	boost::asio::deadline_timer* m_pTimer;
	bool m_logined;
	char m_ip[16];
	unsigned short m_port;
	EMT_PROTOCOL_TYPE m_protocol;
};


#endif
