#include <iostream>
#include <string>
#include <cstring>
#include <float.h>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>

#ifdef _MSC_VER
#include <windows.h>
#else
#include <iconv.h>
#endif

#include "./CTP/ThostFtdcTraderApi.h"

#ifdef _MSC_VER
#pragma comment (lib, "./CTP/soptthosttraderapi_se.lib")
#pragma warning(disable : 4996)
#endif

class UTF8toChar
{
public :
	UTF8toChar(const char *utf8_string)
	{
		init(utf8_string);
	}

	UTF8toChar(const std::string& utf8_string)
	{
		init(utf8_string.c_str());
	}

	void init(const char *utf8_string)
	{
		if (0 == utf8_string)
			t_string = 0;
		else if (0 == *utf8_string)
		{
			needFree = false;
//			t_string = ("");
			t_string = 0;
		}
		else if ( isPureAscii(utf8_string))
		{
			needFree = false;
			t_string = (char *)utf8_string;
		}
		else
		{
			// Either TCHAR = Unicode (2 bytes), or utf8_string contains non-ASCII characters.
			// Needs conversion
			needFree = true;

			// Convert to Unicode (2 bytes)
			std::size_t string_len = strlen(utf8_string);
			std::size_t dst_len = string_len * 2 + 2;
#ifdef _MSC_VER
			wchar_t *buffer = new wchar_t[string_len + 1];
			MultiByteToWideChar(CP_UTF8, 0, utf8_string, -1, buffer, (int)string_len + 1);
			buffer[string_len] = 0;

			t_string = new char[string_len * 2 + 2];
			WideCharToMultiByte(CP_ACP, 0, buffer, -1, t_string, (int)dst_len, 0, 0);
			t_string[string_len * 2 + 1] = 0;
			delete[] buffer;
#else
			iconv_t cd;
			t_string = new char[dst_len];
			char* p = t_string;
			cd = iconv_open("gb2312", "utf-8");
			if (cd != 0)
			{
				memset(t_string, 0, dst_len);
				iconv(cd, (char**)&utf8_string, &string_len, &p, &dst_len);
				iconv_close(cd);
				t_string[dst_len] = '\0';
			}
#endif
		}
	}

	operator const char*()
	{
		return t_string;
	}

	const char* c_str()
	{
		return t_string;
	}

	~UTF8toChar()
	{
		if (needFree)
			delete[] t_string;
	}

private :
	char *t_string;
	bool needFree;

	//
	// helper utility to test if a string contains only ASCII characters
	//
	bool isPureAscii(const char *s)
	{
		while (*s != 0) { if (*(s++) & 0x80) return false; }
		return true;
	}

	//disable assignment
	UTF8toChar(const UTF8toChar &rhs);
	UTF8toChar &operator=(const UTF8toChar &rhs);
};

class ChartoUTF8
{
public :
	ChartoUTF8(const std::string& str)
	{
		init(str.c_str());
	}

	ChartoUTF8(const char *t_string)
	{
		init(t_string);
	}

	void init(const char *t_string)
	{
		if (0 == t_string)
			utf8_string = 0;
		else if (0 == *t_string)
		{
//			utf8_string = "";
			utf8_string = 0;
			needFree = false;
		}
		else if (isPureAscii((char *)t_string))
		{
			utf8_string = (char *)t_string;
			needFree = false;
		}
		else
		{

			needFree = true;

			std::size_t string_len = strlen(t_string);
			std::size_t dst_len = string_len * 5;
#ifdef _MSC_VER		

			// Convert to Unicode if not already in unicode.
			wchar_t *w_string = new wchar_t[string_len + 1];
			MultiByteToWideChar(CP_ACP, 0, t_string, -1, w_string, (int)string_len + 1);
			w_string[string_len] = 0;

			// Convert from Unicode (2 bytes) to UTF8
			utf8_string = new char[dst_len];
			WideCharToMultiByte(CP_UTF8, 0, w_string, -1, utf8_string, (int)dst_len, 0, 0);
			utf8_string[string_len * 3] = 0;

			if (w_string != (wchar_t *)t_string)
				delete[] w_string;
#else
			iconv_t cd;
			utf8_string = new char[dst_len];
			char* p = utf8_string;
			cd = iconv_open("utf-8", "gb2312");
			if (cd != 0)
			{
				memset(utf8_string, 0, dst_len);
				iconv(cd, (char**)&t_string, &string_len, &p, &dst_len);
				iconv_close(cd);
			}
#endif
		}
	}

	operator const char*()
	{
		return utf8_string;
	}

	const char* c_str() const
	{
		return utf8_string;
	}

	~ChartoUTF8()
	{
		if (needFree)
			delete[] utf8_string;
	}

private :
	char *utf8_string;
	bool needFree;

	//
	// helper utility to test if a string contains only ASCII characters
	//
	bool isPureAscii(const char *s)
	{
		while (*s != 0) { if (*(s++) & 0x80) return false; }
		return true;
	}

	//disable assignment
	ChartoUTF8(const ChartoUTF8 &rhs);
	ChartoUTF8 &operator=(const ChartoUTF8 &rhs);
};


class semaphore
{
public:
	semaphore(int value = 1) :count(value) {}

	void wait()
	{
		std::unique_lock<std::mutex> lck(mt);
		if (--count < 0)	//资源不足挂起线程
			cv.wait(lck);
	}

	void signal()
	{
		std::unique_lock<std::mutex> lck(mt);
		if (++count <= 0)	//有线程挂起，唤醒一个
			cv.notify_one();
	}

private:
	int count;
	std::mutex mt;
	std::condition_variable cv;
};

semaphore _semaphore(0);

class CApplication : public CThostFtdcTraderSpi
{
public:
	CApplication(std::string host, std::string broker, std::string user, std::string password, std::string appid, std::string authcode) :
		m_host(host),
		m_broker(broker),
		m_user(user),
		m_password(password),
		m_appid(appid),
		m_authcode(authcode)
	{
		m_pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi();
		m_pUserApi->RegisterSpi(this);
	}

	~CApplication()
	{
	}

	const std::string direction_to_string(TThostFtdcDirectionType Direction)
	{
		std::string str = Direction == THOST_FTDC_D_Buy ? "买" : "卖";
		return std::move(str);
	}

	const std::string posidirection_to_string(TThostFtdcPosiDirectionType Direction)
	{
		std::string str = Direction == THOST_FTDC_PD_Long ? "买" : "卖";
		return std::move(str);
	}

	double double_format(double value)
	{
		return value==DBL_MAX?0.0:value;
	}

	int Run()
	{
		std::cout << "CTP交易接口开始运行" << std::endl;
		std::cout << "交易服务器：" << m_host << std::endl;
		m_pUserApi->RegisterFront((char*)m_host.c_str());

		std::cout << "SubscribePrivateTopic:THOST_TERT_QUICK" << std::endl;
		m_pUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);

		std::cout << "SubscribePublicTopic:THOST_TERT_QUICK" << std::endl;
		m_pUserApi->SubscribePublicTopic(THOST_TERT_QUICK);

		std::cout << "Init ..." << std::endl;
		m_pUserApi->Init();

		return 0;
	}

	//连接成功
	void OnFrontConnected()
	{
		std::cout << __FUNCTION__ << std::endl;

		CThostFtdcReqAuthenticateField Req;
		memset(&Req, 0x00, sizeof(Req));

#ifdef _MSC_VER
		strncpy_s(Req.BrokerID, m_broker.c_str(), sizeof(Req.BrokerID) - 1);
		strncpy_s(Req.UserID, m_user.c_str(), sizeof(Req.UserID) - 1);
		strncpy_s(Req.AuthCode, m_authcode.c_str(), sizeof(Req.AuthCode) - 1);
		strncpy_s(Req.AppID, m_appid.c_str(), sizeof(Req.AppID) - 1);
#else
		strncpy(Req.BrokerID, m_broker.c_str(), sizeof(Req.BrokerID) - 1);
		strncpy(Req.UserID, m_user.c_str(), sizeof(Req.UserID) - 1);
		strncpy(Req.AuthCode, m_authcode.c_str(), sizeof(Req.AuthCode) - 1);
		strncpy(Req.AppID, m_appid.c_str(), sizeof(Req.AppID) - 1);
#endif
		m_pUserApi->ReqAuthenticate(&Req, 0);
		std::cout << "ReqAuthenticate ..." << std::endl;
	}

	//连接断开
	void OnFrontDisconnected(int nReason)
	{
		printf("Disconnected.\n");
	}

	///认证响应
	void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo->ErrorID != 0)
			printf("%s: %s\n", __FUNCTION__, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
		else
			printf("Authenticate succeeded.\n");

		// 登录
		CThostFtdcReqUserLoginField Req;
		memset(&Req, 0x00, sizeof(Req));

#ifdef _MSC_VER
		strncpy_s(Req.BrokerID, m_broker.c_str(), sizeof(Req.BrokerID) - 1);
		strncpy_s(Req.UserID, m_user.c_str(), sizeof(Req.UserID) - 1);
		strncpy_s(Req.Password, m_password.c_str(), sizeof(Req.Password) - 1);
#else
		strncpy(Req.BrokerID, m_broker.c_str(), sizeof(Req.BrokerID) - 1);
		strncpy(Req.UserID, m_user.c_str(), sizeof(Req.UserID) - 1);
		strncpy(Req.Password, m_password.c_str(), sizeof(Req.Password) - 1);
#endif

		m_pUserApi->ReqUserLogin(&Req, 0);
		std::cout << "Login ...\n";
	}

	//登录应答
	void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("Login failed. %d - %s\n", pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}
		printf("Login succeeded: TradingDay: %s, FrontID = %d, SessionID = %d\n", 
			pRspUserLogin->TradingDay, pRspUserLogin->FrontID, pRspUserLogin->SessionID);
		
		m_nOrderRef = atol(pRspUserLogin->MaxOrderRef);

		// 确认结算单
		CThostFtdcSettlementInfoConfirmField SettlementInfoConfirmField;
		memset(&SettlementInfoConfirmField, 0x00, sizeof(SettlementInfoConfirmField));

#ifdef _MSC_VER
		strncpy_s(SettlementInfoConfirmField.BrokerID, pRspUserLogin->BrokerID, sizeof(SettlementInfoConfirmField.BrokerID) - 1);
		strncpy_s(SettlementInfoConfirmField.InvestorID, pRspUserLogin->UserID, sizeof(SettlementInfoConfirmField.InvestorID) - 1);
		strncpy_s(SettlementInfoConfirmField.ConfirmDate, pRspUserLogin->TradingDay, sizeof(SettlementInfoConfirmField.ConfirmDate) - 1);
		strncpy_s(SettlementInfoConfirmField.ConfirmTime, pRspUserLogin->LoginTime, sizeof(SettlementInfoConfirmField.ConfirmTime) - 1);
#else
		strncpy(SettlementInfoConfirmField.BrokerID, pRspUserLogin->BrokerID, sizeof(SettlementInfoConfirmField.BrokerID) - 1);
		strncpy(SettlementInfoConfirmField.InvestorID, pRspUserLogin->UserID, sizeof(SettlementInfoConfirmField.InvestorID) - 1);
		strncpy(SettlementInfoConfirmField.ConfirmDate, pRspUserLogin->TradingDay, sizeof(SettlementInfoConfirmField.ConfirmDate) - 1);
		strncpy(SettlementInfoConfirmField.ConfirmTime, pRspUserLogin->LoginTime, sizeof(SettlementInfoConfirmField.ConfirmTime) - 1);
#endif
		m_pUserApi->ReqSettlementInfoConfirm(&SettlementInfoConfirmField, 0);
		std::cout << "Confirm Settlement ...\n";

		_semaphore.signal();
	}

	///投资者结算结果确认响应
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
	{
		if (pRspInfo && pRspInfo->ErrorID != 0)
			printf("%s: %d - %s\n", __FUNCTION__, pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());

		if (pSettlementInfoConfirm)
			printf("%s: BrokerID: %s, InvestorID: %s, currencyID: %s\n", __FUNCTION__, 
				pSettlementInfoConfirm->BrokerID, pSettlementInfoConfirm->InvestorID, pSettlementInfoConfirm->CurrencyID);
		else
			printf("%s: Confirm failed.\n", __FUNCTION__);

		if (bIsLast) {
//		_semaphore.wait();
			printf("Query completed.\n");
		}
	};

	// 下单
	int OrderInsert(const char* ExchangeID, const char* InstrumentID, TThostFtdcDirectionType Direction, TThostFtdcOffsetFlagType OffsetFlag, double Price, unsigned int Qty, TThostFtdcHedgeFlagType HedgeFlag, TThostFtdcOrderPriceTypeType OrderPriceType)
	{
		CThostFtdcInputOrderField Req;

		memset(&Req, 0x00, sizeof(Req));

#ifdef _MSC_VER
		strncpy_s(Req.BrokerID, m_broker.c_str(), sizeof(Req.BrokerID) - 1);
		strncpy_s(Req.InvestorID, m_user.c_str(), sizeof(Req.InvestorID) - 1);
		strncpy_s(Req.InstrumentID, InstrumentID, sizeof(Req.InstrumentID) - 1);
		strncpy_s(Req.ExchangeID, ExchangeID, sizeof(Req.ExchangeID) - 1);
#else
		strncpy(Req.BrokerID, m_broker.c_str(), sizeof(Req.BrokerID) - 1);
		strncpy(Req.InvestorID, m_user.c_str(), sizeof(Req.InvestorID) - 1);
		strncpy(Req.InstrumentID, InstrumentID, sizeof(Req.InstrumentID) - 1);
		strncpy(Req.ExchangeID, ExchangeID, sizeof(Req.ExchangeID) - 1);
#endif

		Req.Direction = Direction;
		Req.CombOffsetFlag[0] = OffsetFlag;
		Req.CombHedgeFlag[0] = HedgeFlag;	// THOST_FTDC_HF_Speculation;
		Req.VolumeTotalOriginal = Qty;
		Req.LimitPrice = Price;
		Req.OrderPriceType = OrderPriceType; // THOST_FTDC_OPT_LimitPrice;

#ifdef _MSC_VER
		sprintf_s(Req.OrderRef, "%d", m_nOrderRef++);
#else		
		sprintf(Req.OrderRef, "%d", m_nOrderRef++);
#endif

		Req.TimeCondition = THOST_FTDC_TC_GFD;
		if (Req.OrderPriceType == THOST_FTDC_OPT_AnyPrice)
			Req.TimeCondition = THOST_FTDC_TC_IOC;
		Req.VolumeCondition = THOST_FTDC_VC_AV;
		Req.MinVolume = 1;
		Req.ContingentCondition = THOST_FTDC_CC_Immediately;
		Req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		Req.IsAutoSuspend = 0;
		Req.UserForceClose = 0;
		Req.RequestID = 1; 	// Only used by User

#ifdef _MSC_VER
		strncpy_s(Req.InvestUnitID, "XXXX", sizeof(Req.InvestUnitID) - 1); // Only used by User
#else	
		strncpy(Req.InvestUnitID, "XXXX", sizeof(Req.InvestUnitID) - 1); // Only used by User
#endif

		return m_pUserApi->ReqOrderInsert(&Req, 0);
	}

	void insertOrder()
	{
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		TThostFtdcDirectionType direct;
		TThostFtdcOffsetFlagType type;
		std::string ExchangeID;
		std::string InstrumentID;
		double price;
		TThostFtdcHedgeFlagType HedgeFlag;
		TThostFtdcOrderPriceTypeType OrderPriceType;

		std::cout << "请输入: ExchangeID; InstrumentID;" << std::endl;
		std::cout << "买卖方向:0-买,1-卖; 开平方向:0-开仓,1-平仓,2-强平,3-平今,4-平昨,5-强减;" << std::endl;
		std::cout << "价格:价格(0.0001为最小单位); " << "投机套保标志:1-投机,2-套利,3-套保,4-备兑,5-做市商;" << std::endl;
		std::cout << "报单价格条件:1-任意价,2-限价,3-最优价,4-最新价,5-最新价浮动上浮1个ticks,6-最新价浮动上浮2个ticks," << std::endl;
		std::cout << "	7-最新价浮动上浮3个ticks,8-最新价浮动下浮1个ticks,9-最新价浮动下浮2个ticks,10-最新价浮动下浮3个ticks;" << std::endl;
		std::cin >> ExchangeID >> InstrumentID >> direct >> type >> price >> HedgeFlag >> OrderPriceType;
		OrderInsert(ExchangeID.c_str(), InstrumentID.c_str(), direct, type, price, 1, HedgeFlag, OrderPriceType);
	}

	// 下单应答
	void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("OnRspOrderInsert. %d - %s\n", pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}
		printf("OnRspOrderInsert:InstrumentID:%s,ExchangeID:%s,VolumeTotalOriginal:%d,LimitPrice:%lf,RequestID:%d,InvestUnitID:%s\n", pInputOrder->InstrumentID, pInputOrder->ExchangeID, pInputOrder->VolumeTotalOriginal, pInputOrder->LimitPrice, pInputOrder->RequestID, pInputOrder->InvestUnitID);
	}

	// 撤单
	void CancelOrder()
	{
		std::string InstrumentID, OrderSysID;
		std::cout << "请输入: InstrumentID; OrderSysID" << std::endl;
		std::cin >> InstrumentID >> OrderSysID;
		OrderCancel("SSE", InstrumentID.c_str(), OrderSysID.c_str());
	}

	int OrderCancel(const char* ExchangeID, const char* InstrumentID, const char* OrderSysID)
	{
		CThostFtdcInputOrderActionField Req;

		memset(&Req, 0x00, sizeof(Req));

#ifdef _MSC_VER
		strncpy_s(Req.BrokerID, m_broker.c_str(), sizeof(Req.BrokerID) - 1);
		strncpy_s(Req.InvestorID, m_user.c_str(), sizeof(Req.InvestorID) - 1);
		strncpy_s(Req.InstrumentID, InstrumentID, sizeof(Req.InstrumentID) - 1);
		strncpy_s(Req.ExchangeID, ExchangeID, sizeof(Req.ExchangeID) - 1);
		strncpy_s(Req.OrderSysID, OrderSysID, sizeof(Req.OrderSysID) - 1);
#else
		strncpy(Req.BrokerID, m_broker.c_str(), sizeof(Req.BrokerID) - 1);
		strncpy(Req.InvestorID, m_user.c_str(), sizeof(Req.InvestorID) - 1);
		strncpy(Req.InstrumentID, InstrumentID, sizeof(Req.InstrumentID) - 1);
		strncpy(Req.ExchangeID, ExchangeID, sizeof(Req.ExchangeID) - 1);
		strncpy(Req.OrderSysID, OrderSysID, sizeof(Req.OrderSysID) - 1);
#endif

		//Req.FrontID = 100;
		//Req.SessionID = 1;
		//strncpy_s(Req.OrderRef, "111", sizeof(Req.OrderRef) - 1);
		Req.ActionFlag = THOST_FTDC_AF_Delete;

		return m_pUserApi->ReqOrderAction(&Req, 0);
	}

	// 撤单应答
	void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("OnRspOrderAction. %d - %s\n", pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}
		printf("OnRspOrderAction:InstrumentID:%s,ExchangeID:%s,OrderSysID:%s,FrontID:%d,SessionID:%d,OrderRef:%s,RequestID:%d,InvestUnitID:%s\n",pInputOrderAction->InstrumentID,pInputOrderAction->ExchangeID,pInputOrderAction->OrderSysID, pInputOrderAction->FrontID, pInputOrderAction->SessionID, pInputOrderAction->OrderRef,pInputOrderAction->RequestID,pInputOrderAction->InvestUnitID);
	}


	// 查询品种
	void qryProduct()
	{
//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		printf("查询品种 ...\n");
		CThostFtdcQryProductField QryProduct = { 0 };
		m_pUserApi->ReqQryProduct(&QryProduct, 0);
		_semaphore.wait();
	}

	/*产品查询应答*/
	void OnRspQryProduct(CThostFtdcProductField* pProduct, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pProduct)
			printf("OnRspQryProduct:ProductID: %s,	ProductName: %s, ProductClass: %c, ExchangeID: %s\n", pProduct->ProductID, 
			ChartoUTF8(pProduct->ProductName).c_str(), pProduct->ProductClass, pProduct->ExchangeID);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	// 查询交易所
	void qryExchange()
	{
		printf("查询交易所 ...\n");
		CThostFtdcQryExchangeField QryExchange = { 0 };
		m_pUserApi->ReqQryExchange(&QryExchange, 0);
	}

	/*交易所查询应答*/
	void OnRspQryExchange(CThostFtdcExchangeField* pExchange, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pExchange)
		{
			printf("OnRspQryExchange:ExchangeID:%s,ExchangeName:%s\n", pExchange->ExchangeID, ChartoUTF8(pExchange->ExchangeName).c_str());
		}

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	/*投资者手续费率查询应答*/
	void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField* pInstrumentCommissionRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInstrumentCommissionRate)
			printf("OnRspQryInstrumentCommissionRate:ExchangeID:%s,InstrumentID:%s,OpenRatioByMoney:%lf,CloseRatioByMoney:%lf,CloseTodayRatioByMoney:%lf,OpenRatioByVolume:%lf,CloseRatioByVolume:%lf,CloseTodayRatioByVolume:%lf\n", 
				pInstrumentCommissionRate->ExchangeID, pInstrumentCommissionRate->InstrumentID,
				pInstrumentCommissionRate->OpenRatioByMoney, pInstrumentCommissionRate->CloseRatioByMoney, pInstrumentCommissionRate->CloseTodayRatioByMoney,
				pInstrumentCommissionRate->OpenRatioByVolume, pInstrumentCommissionRate->CloseRatioByVolume, pInstrumentCommissionRate->CloseTodayRatioByVolume);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	/*申报手续费率查询应答*/
	void OnRspQryInstrumentOrderCommRate(CThostFtdcInstrumentOrderCommRateField* pInstrumentOrderCommRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInstrumentOrderCommRate)
			printf("OnRspQryInstrumentOrderCommRate:ExchangeID:%s,InstrumentID:%s,OrderCommByVolume:%lf,OrderActionCommByVolume:%lf\n",
				pInstrumentOrderCommRate->ExchangeID, pInstrumentOrderCommRate->InstrumentID,
				pInstrumentOrderCommRate->OrderCommByVolume, pInstrumentOrderCommRate->OrderActionCommByVolume);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	/*投资者保证金率查询应答*/
	void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField* pInstrumentMarginRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInstrumentMarginRate)
			printf("OnRspQryInstrumentMarginRate:ExchangeID:%s,InstrumentID:%s,LongMarginRatioByMoney:%lf,LongMarginRatioByVolume:%lf,ShortMarginRatioByMoney:%lf,ShortMarginRatioByVolume:%lf\n",
				pInstrumentMarginRate->ExchangeID, pInstrumentMarginRate->InstrumentID,
				pInstrumentMarginRate->LongMarginRatioByMoney, pInstrumentMarginRate->LongMarginRatioByVolume, pInstrumentMarginRate->ShortMarginRatioByMoney,
				pInstrumentMarginRate->ShortMarginRatioByVolume);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	// 查询交易所保证金率
	void qryExchangeMarginRate()
	{
		std::cout << "查询交易所保证金率, 请输入InstrumentID and HedgeFlag: " << std::endl;;

		CThostFtdcQryExchangeMarginRateField QryExchangeMarginRate = { 0 };
		std::cin >> QryExchangeMarginRate.InstrumentID >> QryExchangeMarginRate.HedgeFlag;

		strncpy(QryExchangeMarginRate.BrokerID, "8888", sizeof(QryExchangeMarginRate.BrokerID) - 1);
		strncpy(QryExchangeMarginRate.ExchangeID, "SSE", sizeof(QryExchangeMarginRate.ExchangeID) - 1);
		strncpy(QryExchangeMarginRate.InstrumentID, m_user.c_str(), sizeof(QryExchangeMarginRate.InstrumentID) - 1);

		m_pUserApi->ReqQryExchangeMarginRate(&QryExchangeMarginRate, 0);
		_semaphore.wait();
	}

	///请求查询交易所保证金率响应
	void OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField* pExchangeMarginRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pExchangeMarginRate)
			printf("OnRspQryExchangeMarginRate:ExchangeID:%s,InstrumentID:%s,LongMarginRatioByMoney:%lf,LongMarginRatioByVolume:%lf,ShortMarginRatioByMoney:%lf,ShortMarginRatioByVolume:%lf\n",
				pExchangeMarginRate->ExchangeID, pExchangeMarginRate->InstrumentID,
				pExchangeMarginRate->LongMarginRatioByMoney, pExchangeMarginRate->LongMarginRatioByVolume, pExchangeMarginRate->ShortMarginRatioByMoney,
				pExchangeMarginRate->ShortMarginRatioByVolume);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	// 查询合约
	void qryInstrument()
	{
		CThostFtdcQryInstrumentField QryInstrument = { 0 };
		std::cout << "请输入合约ID: " << std::endl;
		std::cin >> QryInstrument.InstrumentID;
		if  (0 == strcmp(QryInstrument.InstrumentID, "0000000"))
		{
			memset(&QryInstrument, 0x00, sizeof(QryInstrument));
		}
		m_pUserApi->ReqQryInstrument(&QryInstrument, 0);
		_semaphore.wait();
	}

	// 查询合约列表
	void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if ((pInstrument)) // 过滤指定合约"
			printf("OnRspQryInstrument:InstrumentID:%s,InstrumentName:%s,ProductID:%s,PriceTick:%lf,UnderlyingInstrID:%s,StrikePrice:%lf,ExchangeID:%s\n", pInstrument->InstrumentID, ChartoUTF8(pInstrument->InstrumentName).c_str(), pInstrument->ProductID, pInstrument->PriceTick, pInstrument->UnderlyingInstrID, pInstrument->StrikePrice, pInstrument->ExchangeID);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	// 查询行情
	void qryDepthMarketdata()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		CThostFtdcQryDepthMarketDataField QryDepthMarketData = { 0 };
		std::cout << "请输入交易所代码, 合约ID: " << std::endl;
		std::cin >> QryDepthMarketData.ExchangeID >> QryDepthMarketData.InstrumentID;
		m_pUserApi->ReqQryDepthMarketData(&QryDepthMarketData, 0);
		_semaphore.wait();
	}

	// 查询行情列表
	void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("OnRspQryDepthMarketData. %d - %s\n", pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}

		if(pDepthMarketData)
			printf("OnRspQryDepthMarketData:InstrumentID:%s,LastPrice:%lf,Volume:%d,Turnover:%lf,OpenPrice:%lf,HighestPrice:%lf,LowestPrice:%lf,UpperLimitPrice:%lf,LowerLimitPrice:%lf,OpenInterest:%lf,PreClosePrice:%lf,PreSettlementPrice:%lf,SettlementPrice:%lf,UpdateTime:%s,ActionDay:%s,TradingDay:%s,ExchangeID:%s\n",
				pDepthMarketData->InstrumentID, double_format(pDepthMarketData->LastPrice), pDepthMarketData->Volume, double_format(pDepthMarketData->Turnover), double_format(pDepthMarketData->OpenPrice),
				double_format(pDepthMarketData->HighestPrice), double_format(pDepthMarketData->LowestPrice), double_format(pDepthMarketData->UpperLimitPrice), double_format(pDepthMarketData->LowerLimitPrice),
				double_format(pDepthMarketData->OpenInterest), double_format(pDepthMarketData->PreClosePrice), double_format(pDepthMarketData->PreSettlementPrice),
				double_format(pDepthMarketData->SettlementPrice), pDepthMarketData->UpdateTime, pDepthMarketData->ActionDay, pDepthMarketData->TradingDay, pDepthMarketData->ExchangeID);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}
//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	// 查询订单
	void qryOrder()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		printf("查询订单 ...\n");
		CThostFtdcQryOrderField QryOrder = { 0 };
		m_pUserApi->ReqQryOrder(&QryOrder, 0);
		_semaphore.wait();
	}

	// 查询委托列表
	void OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("%s: %d - %s\n", __FUNCTION__, pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}

		if(pOrder)
			printf("%s: BrokerID: %s, BrokerOrderSeq: %d, OrderLocalID: %s, InstrumentID: %s, Direction: %s, VolumeTotalOriginal: %d,\r\nLimitPrice: %lf, VolumeTraded: %d, VolumeTotal: %d, OrderSysID: %s, FrontID: %d, SessionID: %d, OrderRef: %s, OrderStatus: %c,\r\nStatusMsg: %s, ExchangeID: %s, InsertTime: %s, ClientID: %s, RequestID: %d, InvestUnitID: %s\n", __FUNCTION__,
				pOrder->BrokerID,pOrder->BrokerOrderSeq,pOrder->OrderLocalID, pOrder->InstrumentID, direction_to_string(pOrder->Direction).c_str(), pOrder->VolumeTotalOriginal, pOrder->LimitPrice, pOrder->VolumeTraded, pOrder->VolumeTotal, pOrder->OrderSysID, pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef, pOrder->OrderStatus, ChartoUTF8(pOrder->StatusMsg).c_str(), pOrder->ExchangeID, pOrder->InsertTime,pOrder->ClientID,pOrder->RequestID,pOrder->InvestUnitID);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

    // 查询成交
	void qryTrade()
	{
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        CThostFtdcQryTradeField QryTrade = { 0 };
        m_pUserApi->ReqQryTrade(&QryTrade, 0);
        _semaphore.wait();
	}

	// 查询成交列表
	void OnRspQryTrade(CThostFtdcTradeField* pTrade, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("%s: %d - %s\n", __FUNCTION__, pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}

		if(pTrade)
			printf("%s: BrokerID: %s, BrokerOrderSeq: %d, OrderLocalID:%s, InstrumentID: %s, Direction: %s, Volume: %d, Price: %lf, \r\n OrderSysID: %s, OrderRef: %s, ExchangeID: %s, TradeTime: %s, ClientID: %s, InvestUnitID:%s\n", __FUNCTION__,
				pTrade->BrokerID, pTrade->BrokerOrderSeq, pTrade->OrderLocalID, pTrade->InstrumentID, direction_to_string(pTrade->Direction).c_str(), pTrade->Volume, pTrade->Price, pTrade->OrderSysID, pTrade->OrderRef, pTrade->ExchangeID, pTrade->TradeTime,pTrade->ClientID,pTrade->InvestUnitID);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	///请求查询投资者
	void qryInvestor()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));	
		CThostFtdcQryInvestorField QryInvestor = { 0 };
		m_pUserApi->ReqQryInvestor(&QryInvestor, 0);
		_semaphore.wait();
	}

	// 查询投资者
	void OnRspQryInvestor(CThostFtdcInvestorField* pInvestor, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("%s: %d - %s\n", __FUNCTION__, pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}

		if (pInvestor)
		{
			printf("%s: InvestorID: %s, BrokerID: %s, InvestorGroupID: %s, InvestorName: %s, IdentifiedCardType: %c, \nIdentifiedCardNo: %s, IsActive: %d, Telephone: %s, Address: %s, \nOpenDate: %s, Mobile: %s, CommModelID: %s, MarginModelID: %s\n", __FUNCTION__,	
				pInvestor->InvestorID, pInvestor->BrokerID, pInvestor->InvestorGroupID, 
				ChartoUTF8(pInvestor->InvestorName).c_str(), pInvestor->IdentifiedCardType, pInvestor->IdentifiedCardNo, pInvestor->IsActive, pInvestor->Telephone, 
				ChartoUTF8(pInvestor->Address).c_str(), pInvestor->OpenDate, pInvestor->Mobile, pInvestor->CommModelID, pInvestor->MarginModelID);
		}
		
		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}	
	}

	// 查询交易编码
	void qryTradingCode()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		CThostFtdcQryTradingCodeField QryTradingCode = { 0 };
		m_pUserApi->ReqQryTradingCode(&QryTradingCode, 0);
		_semaphore.wait();
	}

	// 查询交易编码
	void OnRspQryTradingCode(CThostFtdcTradingCodeField* pTradingCode, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("%s: %d - %s\n", __FUNCTION__, pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}

		if (pTradingCode)
			printf("%s: InvestorID: %s, BrokerID: %s, ExchangeID: %s, ClientID: %s, IsActive: %d, ClientIDType: %c, \r\nBranchID: %s, BizType: %d, InvestUnitID: %s\n", __FUNCTION__,
				pTradingCode->InvestorID, pTradingCode->BrokerID, pTradingCode->ExchangeID, pTradingCode->ClientID, pTradingCode->IsActive, pTradingCode->ClientIDType, pTradingCode->BranchID, pTradingCode->BizType, pTradingCode->InvestUnitID);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	
	// 查询转帐银行
	void qryTransferBank()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		CThostFtdcQryTransferBankField QryTransferBank = { 0 };
		m_pUserApi->ReqQryTransferBank(&QryTransferBank, 0);
		_semaphore.wait();
	}

	void OnRspQryTransferBank(CThostFtdcTransferBankField* pTransferBank, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("%s: %d - %s\n", __FUNCTION__, pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}

		if (pTransferBank)
			printf("%s: BankID: %s, BankBrchID: %s, BankName: %s\n", __FUNCTION__, pTransferBank->BankID, pTransferBank->BankBrchID, ChartoUTF8(pTransferBank->BankName).c_str());
		else
			printf("%s: NULL\n", __FUNCTION__);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	// 查询期权交易成本
	void qryOptionInstrTradeCost()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		CThostFtdcQryOptionInstrTradeCostField QryOptionInstrTradeCost = { 0 };
		m_pUserApi->ReqQryOptionInstrTradeCost(&QryOptionInstrTradeCost, 0);
		_semaphore.wait();
	}

	void OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField* pOptionInstrTradeCost, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("%s: %d - %s\n", __FUNCTION__, pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}

		if (pOptionInstrTradeCost)
			printf("%s: BrokerID: %s, InvestorID: %s, InstrumentID: %s, HedgeFlag: %c, FixedMargin: %lf, MiniMargin: %lf, Royalty: %lf, ExchFixedMargin: %lf, ExchMiniMargin: %lf, ExchangeID: %s, InvestUnitID: %s\n", __FUNCTION__,
				pOptionInstrTradeCost->BrokerID, pOptionInstrTradeCost->InvestorID, pOptionInstrTradeCost->InstrumentID, pOptionInstrTradeCost->HedgeFlag, pOptionInstrTradeCost->FixedMargin, pOptionInstrTradeCost->MiniMargin, pOptionInstrTradeCost->Royalty, pOptionInstrTradeCost->ExchFixedMargin, pOptionInstrTradeCost->ExchMiniMargin, pOptionInstrTradeCost->ExchangeID, pOptionInstrTradeCost->InvestUnitID);
		else
			printf("%s: NULL\n", __FUNCTION__);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	// 查询询价
	void qryForQuote()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		CThostFtdcQryForQuoteField QryForQuote = { 0 };
		m_pUserApi->ReqQryForQuote(&QryForQuote, 0);
		_semaphore.wait();
	}

	void OnRspQryForQuote(CThostFtdcForQuoteField* pForQuote, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("%s: %d - %s\n", __FUNCTION__, pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}

		if (pForQuote)
			printf("%s: BrokerID: %s, InvestorID: %s, InstrumentID: %s, ForQuoteRef: %s, UserID: %s, ForQuoteLocalID: %s, ExchangeID: %s, ParticipantID %s, ClientID: %s, ExchangeInstID: %s, TraderID: %s, InstallID: %d, InsertDate: %s, InsertTime: %s, ForQuoteStatus: %c, FrontID: %d, SessionID: %d, StatusMsg: %s, ActiveUserID: %s, BrokerForQutoSeq: %d, InvestUnitID: %s\n", __FUNCTION__, 
				pForQuote->BrokerID, pForQuote->InvestorID, pForQuote->InstrumentID, pForQuote->ForQuoteRef, pForQuote->UserID, pForQuote->ForQuoteLocalID, pForQuote->ExchangeID, pForQuote->ParticipantID, pForQuote->ClientID, pForQuote->ExchangeInstID, pForQuote->TraderID, pForQuote->InstallID, pForQuote->InsertDate, pForQuote->InsertTime, pForQuote->ForQuoteStatus, pForQuote->FrontID, pForQuote->SessionID, ChartoUTF8(pForQuote->StatusMsg).c_str(), pForQuote->ActiveUserID, pForQuote->BrokerForQutoSeq, pForQuote->InvestUnitID);
		else
			printf("%s: NULL\n", __FUNCTION__);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}


	// 查询经纪公司交易算法
	void qryBrokerTradingAlgos()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		CThostFtdcQryBrokerTradingAlgosField QryBrokerTradingAlgos = { 0 };
		m_pUserApi->ReqQryBrokerTradingAlgos(&QryBrokerTradingAlgos, 0);
		_semaphore.wait();
	}

	void OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField* pBrokerTradingAlgos, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("%s: %d - %s\n", __FUNCTION__, pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}

		if (pBrokerTradingAlgos)
			printf("%s: BrokerID: %s, ExchangeID: %s, InstrumentID: %s, HandlePositionAlgoID: %c, FindMarginRateAlgoID: %c, HandleTradingAccountAlgoID: %c\n", __FUNCTION__,
				pBrokerTradingAlgos->BrokerID, pBrokerTradingAlgos->ExchangeID, pBrokerTradingAlgos->InstrumentID, pBrokerTradingAlgos->HandlePositionAlgoID, pBrokerTradingAlgos->FindMarginRateAlgoID, pBrokerTradingAlgos->HandleTradingAccountAlgoID);
		else
			printf("%s: NULL\n", __FUNCTION__);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}


	// 查询持仓
	void qryInvestorPositionField()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		CThostFtdcQryInvestorPositionField QryInvestorPosition = { 0 };
		m_pUserApi->ReqQryInvestorPosition(&QryInvestorPosition, 0);
		_semaphore.wait();
	}

	// 查询持仓
	void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("%s: %d - %s\n", __FUNCTION__, pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}

		if (pInvestorPosition)
			printf("%s: InstrumentID: %s, PosiDirection: %s, HedgeFlag: %c, Position: %d, YdPosition: %d, TodayPosition: %d, PositionCost: %lf,OpenCost: %lf, ExchangeID: %s\n", __FUNCTION__,
				pInvestorPosition->InstrumentID, posidirection_to_string(pInvestorPosition->PosiDirection).c_str(), pInvestorPosition->HedgeFlag, pInvestorPosition->Position, pInvestorPosition->YdPosition, pInvestorPosition->TodayPosition, pInvestorPosition->PositionCost, pInvestorPosition->OpenCost, pInvestorPosition->ExchangeID);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

    // 持仓明细
	void qryInvestorPositionDetail()
	{
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        printf("持仓明细 ...\n");
        CThostFtdcQryInvestorPositionDetailField QryInvestorPositionDetail = { 0 };
        m_pUserApi->ReqQryInvestorPositionDetail(&QryInvestorPositionDetail, 0);
        _semaphore.wait();
	}

	// 查询持仓明细
	void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField* pInvestorPositionDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("OnRspQryInvestorPositionDetail. %d - %s\n", pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());
			return;
		}

		if (pInvestorPositionDetail)
			printf("OnRspQryInvestorPositionDetail:InstrumentID:%s,Direction:%s,HedgeFlag:%c,Volume:%d,OpenDate:%s,OpenPrice:%lf,Margin:%lf,ExchangeID:%s\n",
				pInvestorPositionDetail->InstrumentID, direction_to_string(pInvestorPositionDetail->Direction).c_str(), pInvestorPositionDetail->HedgeFlag, pInvestorPositionDetail->Volume, pInvestorPositionDetail->OpenDate, pInvestorPositionDetail->OpenPrice, pInvestorPositionDetail->Margin, pInvestorPositionDetail->ExchangeID);

		if (bIsLast) {
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	// 查询资金
	void qryTradingAccount()
	{
//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		CThostFtdcQryTradingAccountField QryTradingAccount = { 0 };
		m_pUserApi->ReqQryTradingAccount(&QryTradingAccount, 0);
		_semaphore.signal();
	}

	///请求查询资金账户响应
	void OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pTradingAccount)
			printf("%s: AccountID: %s, Available: %lf, FrozenCash: %lf, FrozenCommission: %lf\n", __FUNCTION__,
				pTradingAccount->AccountID, pTradingAccount->Available, pTradingAccount->FrozenCash, pTradingAccount->FrozenCommission);

		if (bIsLast)
		{
			printf("Query completed.\n");
			_semaphore.signal();
		}
	}

	// 委托回报
	void OnRtnOrder(CThostFtdcOrderField* pOrder)
	{
		printf("OnRtnOrder:BrokerID:%s,BrokerOrderSeq:%d,OrderLocalID:%s,InstrumentID:%s,Direction:%s,VolumeTotalOriginal:%d,LimitPrice:%lf,VolumeTraded:%d,VolumeTotal:%d,OrderSysID:%s,FrontID:%d,SessionID:%d,OrderRef:%s,OrderStatus:%c,StatusMsg:%s,ExchangeID:%s,InsertTime:%s,ClientID:%s,RequestID:%d,InvestUnitID:%s\n",
			pOrder->BrokerID, pOrder->BrokerOrderSeq, pOrder->OrderLocalID, pOrder->InstrumentID, direction_to_string(pOrder->Direction).c_str(), pOrder->VolumeTotalOriginal, pOrder->LimitPrice, pOrder->VolumeTraded, pOrder->VolumeTotal, pOrder->OrderSysID, pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef, pOrder->OrderStatus, ChartoUTF8(pOrder->StatusMsg).c_str(), pOrder->ExchangeID, pOrder->InsertTime,pOrder->ClientID,pOrder->RequestID,pOrder->InvestUnitID);
	}

	// 成交回报
	void OnRtnTrade(CThostFtdcTradeField* pTrade)
	{
		printf("OnRtnTrade:BrokerID:%s,BrokerOrderSeq:%d,OrderLocalID:%s,InstrumentID:%s,Direction:%s,Volume:%d,Price:%lf,OrderSysID:%s,OrderRef:%s,ExchangeID:%s,TradeTime:%s,ClientID:%s,InvestUnitID:%s\n",
			pTrade->BrokerID, pTrade->BrokerOrderSeq, pTrade->OrderLocalID, pTrade->InstrumentID, direction_to_string(pTrade->Direction).c_str(), pTrade->Volume, pTrade->Price, pTrade->OrderSysID, pTrade->OrderRef, pTrade->ExchangeID, pTrade->TradeTime,pTrade->ClientID,pTrade->InvestUnitID);
	}

	// 报单错误
	void OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0)
			printf("OnErrRtnOrderInsert. %d - %s\n", pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());

		if(pInputOrder)
			printf("OnErrRtnOrderInsert:OrderRef:%s,InstrumentID:%s,Direction:%s,VolumeTotalOriginal:%d,LimitPrice:%lf,ExchangeID:%s,RequestID:%d,InvestUnitID:%s\n",
				pInputOrder->OrderRef, pInputOrder->InstrumentID, direction_to_string(pInputOrder->Direction).c_str(), pInputOrder->VolumeTotalOriginal, pInputOrder->LimitPrice, pInputOrder->ExchangeID, pInputOrder->RequestID, pInputOrder->InvestUnitID);
	}

	// 撤单错误回报
	void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0)
			printf("OnErrRtnOrderAction. %d - %s\n", pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());

		if (pOrderAction)
			printf("OnErrRtnOrderAction:FrontID:%d,SessionID:%d,OrderRef:%s,InstrumentID:%s,OrderActionRef:%d,OrderSysID:%s,LimitPrice:%lf,ExchangeID:%s,RequestID:%d,InvestUnitID:%s\n",
				pOrderAction->FrontID, pOrderAction->SessionID, pOrderAction->OrderRef, pOrderAction->InstrumentID, pOrderAction->OrderActionRef, pOrderAction->OrderSysID, pOrderAction->LimitPrice, pOrderAction->ExchangeID, pOrderAction->RequestID, pOrderAction->InvestUnitID);
	}

	///查询投资者结算结果
	void ReqQrySettlementInfo()
	{
		std::cout << __FUNCTION__ << std::endl;

		CThostFtdcQrySettlementInfoField QrySettlementInfo = { 0 };
		strncpy(QrySettlementInfo.BrokerID, "8888", sizeof(QrySettlementInfo.BrokerID) - 1);
		strncpy(QrySettlementInfo.InvestorID, "333306255", sizeof(QrySettlementInfo.InvestorID) - 1);
		strncpy(QrySettlementInfo.TradingDay, m_pUserApi->GetTradingDay(), sizeof(QrySettlementInfo.TradingDay) - 1);
		strncpy(QrySettlementInfo.AccountID, "333306255", sizeof(QrySettlementInfo.AccountID) - 1);
		
		m_pUserApi->ReqQrySettlementInfo(&QrySettlementInfo, 0);
		_semaphore.wait();

	}

	///请求查询投资者结算结果响应
	void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
	{
		if (pRspInfo && pRspInfo->ErrorID != 0)
			printf("OnRspQrySettlementInfo. %d - %s\n", pRspInfo->ErrorID, ChartoUTF8(pRspInfo->ErrorMsg).c_str());

		if (pSettlementInfo)
			printf("OnRspQrySettlementInfo:TradingDay:%s,SettlementID:%d,BrokerID:%s,InvestorID:%s\n",
				pSettlementInfo->TradingDay, pSettlementInfo->SettlementID, pSettlementInfo->BrokerID, pSettlementInfo->InvestorID);

		if (bIsLast) {
			_semaphore.wait();
			printf("Query completed.\n");
		}
	}


public:
	std::string m_host;
	std::string m_broker;
	std::string m_user;
	std::string m_password;
	std::string m_appid;
	std::string m_authcode;
	unsigned int m_nOrderRef;

	CThostFtdcTraderApi* m_pUserApi;
};


void display_usage()
{
	printf("usage:ctpTrader host broker user password appid authcode\n");
	printf("example:ctTrader tcp://118.26.194.228:41205 8888 333306255 888888 client_RanChaoY_1.0 0000000000000000\n");
}

#pragma execution_character_set("utf-8")
//#pragma execution_character_set("gdk-utf8")

int main(int argc, char* argv[])
{
	if (argc != 7) {
		display_usage();
		return -1;
	}

	std::string host(argv[1]), broker(argv[2]), user(argv[3]), password(argv[4]), appid(argv[5]), authcode(argv[6]);

	CApplication Spi(host, broker, user, password, appid, authcode);

	std::cout << "Version : " << Spi.m_pUserApi->GetApiVersion() << std::endl;

	// 启动
	if (Spi.Run() < 0)
		return -1;

	// 等待登录完成
	_semaphore.wait();

	while(true)
	{
		std::cout << "Please pick an action\r\n";
		std::cout << "0. 退出		";
		std::cout << "1. 查询资金	";
		std::cout << "2. 查询订单	";
		std::cout << "3. 查询成交	";
		std::cout << "4. 查询持仓	";
		std::cout << "5. 查询结算单	";
		std::cout << "6. 下单委托\r\n";
		std::cout << "7. NULL	";
		std::cout << "8. 撤单		";
		std::cout << "9. 净头寸交易	";
		std::cout << "a. 查询持仓明细 ";
		std::cout << "b. 查询行情	";
		std::cout << "c. 查询交易所	";
		std::cout << "d. 查询保证金率\r\n";
		std::cout << "e. 查询品种	";
		std::cout << "f. 查询合约	";
		std::cout << "g. 查询投资者	";
		std::cout << "h. 查询交易编码	";
		std::cout << "i. 查询转帐银行	";
		std::cout << "j. 查询期权交易成本	";
		std::cout << "k. 查询询价\r\n";
		std::cout << "l. 查询经纪公司交易算法	\n";

		char cmd;
		std::cin >> cmd;

		switch (cmd)
		{
			case '1':	// 查询资金
				Spi.qryTradingAccount();
				break;
			case '2': 	// 查询订单
				Spi.qryOrder();
				break;
			case '3': 	// 查询成交
				Spi.qryTrade();
				break;
			case '4': // 查询持仓
				Spi.qryInvestorPositionField();
				break;
			case '5': // 查询结算单
				Spi.ReqQrySettlementInfo();
				break;
			case '6': // 下单委托
				Spi.insertOrder();
				break;
			case '8': 
				Spi.CancelOrder();
				break;
			case '9':
	//			bSucc = trader->entrustLmt(true);
				break;
			case '0': 
				break;
			case 'a':
				Spi.qryInvestorPositionDetail();
				break;
			case 'b': // 查询行情
				Spi.qryDepthMarketdata();
				break;
			case 'c':
				Spi.qryExchange();
				break;
			case 'd': // 查询保证金率
				Spi.qryExchangeMarginRate();
				break;
			case 'e':
				Spi.qryProduct();
				break;
			case 'f':
				Spi.qryInstrument();
				break;
			case 'g': // 查询投资者
				Spi.qryInvestor();
				break;
			case 'h': // 查询交易编码
				Spi.qryTradingCode();
				break;
			case 'i': // 查询转帐银行
				Spi.qryTransferBank();
				break;
			case 'j': // 查询期权交易成本
				Spi.qryOptionInstrTradeCost();
				break;
			case 'k': // 查询询价
				Spi.qryForQuote();
				break;
			case 'l': // 查询经纪公司交易算法
				Spi.qryBrokerTradingAlgos();
				break;
			default:
				cmd = 'X';
				break;
		}

		if(cmd == '0')
			break;
	}

	// 查询投资者手续费率
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	printf("查询投资者手续费率 ...\n");
	CThostFtdcQryInstrumentCommissionRateField QryInstrumentCommissionRate = { 0 };
	strncpy(QryInstrumentCommissionRate.BrokerID, broker.c_str(), sizeof(QryInstrumentCommissionRate.BrokerID) - 1);
	strncpy(QryInstrumentCommissionRate.InvestorID, user.c_str(), sizeof(QryInstrumentCommissionRate.InvestorID) - 1);
	strncpy(QryInstrumentCommissionRate.ExchangeID, "SHFE", sizeof(QryInstrumentCommissionRate.ExchangeID) - 1);
	strncpy(QryInstrumentCommissionRate.InstrumentID, "au2403", sizeof(QryInstrumentCommissionRate.InstrumentID) - 1);
//	Spi.m_pUserApi->ReqQryInstrumentCommissionRate(&QryInstrumentCommissionRate, 0);
//	_semaphore.wait();

	// 查询投资者保证金率
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	printf("查询投资者保证金率 ...\n");
	CThostFtdcQryInstrumentMarginRateField QryInstrumentMarginRate = { 0 };
	strncpy(QryInstrumentCommissionRate.BrokerID, broker.c_str(), sizeof(QryInstrumentCommissionRate.BrokerID) - 1);
	strncpy(QryInstrumentCommissionRate.InvestorID, user.c_str(), sizeof(QryInstrumentCommissionRate.InvestorID) - 1);
	strncpy(QryInstrumentMarginRate.ExchangeID, "SHFE", sizeof(QryInstrumentMarginRate.ExchangeID) - 1);
	strncpy(QryInstrumentMarginRate.InstrumentID, "au2203", sizeof(QryInstrumentMarginRate.InstrumentID) - 1);
//	Spi.m_pUserApi->ReqQryInstrumentMarginRate(&QryInstrumentMarginRate, 0);
//	_semaphore.wait();

	// 查询交易所保证金率
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	printf("查询交易所保证金率 ...\n");
	CThostFtdcQryExchangeMarginRateField QryExchangeMarginRate = { 0 };
	strncpy(QryInstrumentCommissionRate.BrokerID, broker.c_str(), sizeof(QryInstrumentCommissionRate.BrokerID) - 1);
	strncpy(QryInstrumentCommissionRate.InvestorID, user.c_str(), sizeof(QryInstrumentCommissionRate.InvestorID) - 1);
	//strncpy(QryExchangeMarginRate.ExchangeID, "SHFE", sizeof(QryExchangeMarginRate.ExchangeID) - 1);
	//strncpy(QryExchangeMarginRate.InstrumentID, "au2203", sizeof(QryExchangeMarginRate.InstrumentID) - 1);
//	Spi.m_pUserApi->ReqQryExchangeMarginRate(&QryExchangeMarginRate, 0);
//	_semaphore.wait();


#if 0
	// 查询申报手续费率（报撤单费率，主要是中金所有此项费用，大连好像要报撤单笔数达到5000笔之上才收）
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	printf("查询申报手续费率 ...\n");
	CThostFtdcQryInstrumentOrderCommRateField QryInstrumentOrderCommRate = { 0 };
	strncpy(QryInstrumentOrderCommRate.BrokerID, broker.c_str(), broker.size());
	strncpy(QryInstrumentOrderCommRate.InvestorID, user.c_str(), user.size());
	strncpy(QryInstrumentOrderCommRate.InstrumentID, "10006480", sizeof("10006480"));
	strncpy(QryInstrumentOrderCommRate.ExchangeID, "SSE", sizeof("SSE"));
	Spi.m_pUserApi->ReqQryInstrumentOrderCommRate(&QryInstrumentOrderCommRate, 0);
	_semaphore.wait();
#endif

	// 如需下单、撤单，放开下面的代码即可
	//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//printf("按任意键下单 ...\n");
	//getchar();
	//Spi.OrderInsert("SHFE", "au2206", THOST_FTDC_D_Buy, THOST_FTDC_OF_Open, 380.0, 3);

	//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//printf("按任意键下单 ...\n");
	//getchar();
	//Spi.OrderInsert("CFFEX", "IF2201", THOST_FTDC_D_Sell, THOST_FTDC_OF_Open, 5000.0, 1);

	//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//printf("按任意键撤单 ...\n");
	//getchar();
	//Spi.OrderCancel("CFFEX", "IF2201", "xxx");

	return 0;
}
