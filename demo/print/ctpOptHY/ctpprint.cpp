#include <iostream>
#include <string>
#include <cstring>
#include <float.h>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>

#include "./CTP/ThostFtdcTraderApi.h"

#ifdef _MSC_VER
#pragma comment (lib, "./CTP/soptthosttraderapi_se.lib")
#pragma warning(disable : 4996)
#endif

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
		m_pUserApi->RegisterFront((char*)m_host.c_str());
		m_pUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);
		m_pUserApi->SubscribePublicTopic(THOST_TERT_QUICK);
		m_pUserApi->Init();

		return 0;
	}

	// 下单
	int OrderInsert(const char* ExchangeID, const char* InstrumentID, TThostFtdcDirectionType Direction, TThostFtdcOffsetFlagType OffsetFlag, double Price, unsigned int Qty)
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
		Req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		Req.VolumeTotalOriginal = Qty;
		Req.LimitPrice = Price;
		Req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;

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

		std::cout << "请输入买卖方向:0-买,1-卖; 开平方向:0-开仓,1-平仓,2-强平,3-平今,4-平昨,5-强减" << std::endl;
		std::cin >> direct >> type;
		OrderInsert("SSE", "10006150", direct, type, _lastPrice, 1);
		//      Spi.OrderInsert("SSE", "10006150", THOST_FTDC_D_Sell, THOST_FTDC_OF_Open, Spi._lastPrice - 0.0001, 1);
		//      Spi.OrderInsert("SSE", "10006150", THOST_FTDC_D_Buy, THOST_FTDC_OF_Open, 0.0001 + Spi._lastPrice, 1);
		//      Spi.OrderInsert("SSE", "10006150", THOST_FTDC_D_Sell, THOST_FTDC_OF_Close, Spi._lastPrice - 0.0001, 1);
	}

	// 撤单
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

	//连接成功
	void OnFrontConnected()
	{
		printf("Connected.\n");
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
			printf("OnRspAuthenticate:%s\n", pRspInfo->ErrorMsg);
		else
			printf("Authenticate succeeded.\n");

		// 登录
		printf("Login ...\n");
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
	}

	//登录应答
	void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("Login failed. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}
		printf("Login succeeded.TradingDay:%s,FrontID=%d,SessionID=%d\n", pRspUserLogin->TradingDay, pRspUserLogin->FrontID, pRspUserLogin->SessionID);
		m_nOrderRef = atol(pRspUserLogin->MaxOrderRef);

		// 确认结算单
		CThostFtdcSettlementInfoConfirmField SettlementInfoConfirmField;

		memset(&SettlementInfoConfirmField, 0x00, sizeof(SettlementInfoConfirmField));

#ifdef _MSC_VER
		strncpy_s(SettlementInfoConfirmField.BrokerID, pRspUserLogin->BrokerID, sizeof(SettlementInfoConfirmField.BrokerID) - 1);
		strncpy_s(SettlementInfoConfirmField.InvestorID, pRspUserLogin->UserID, sizeof(SettlementInfoConfirmField.InvestorID) - 1);
		strncpy_s(SettlementInfoConfirmField.ConfirmDate, pRspUserLogin->TradingDay, sizeof(SettlementInfoConfirmField.ConfirmDate) - 1);
		strncpy_s(SettlementInfoConfirmField.ConfirmTime, pRspUserLogin->LoginTime, sizeof(SettlementInfoConfirmField.ConfirmTime) - 1);
		m_pUserApi->ReqSettlementInfoConfirm(&SettlementInfoConfirmField, 0);
#else
		strncpy(SettlementInfoConfirmField.BrokerID, pRspUserLogin->BrokerID, sizeof(SettlementInfoConfirmField.BrokerID) - 1);
		strncpy(SettlementInfoConfirmField.InvestorID, pRspUserLogin->UserID, sizeof(SettlementInfoConfirmField.InvestorID) - 1);
		strncpy(SettlementInfoConfirmField.ConfirmDate, pRspUserLogin->TradingDay, sizeof(SettlementInfoConfirmField.ConfirmDate) - 1);
		strncpy(SettlementInfoConfirmField.ConfirmTime, pRspUserLogin->LoginTime, sizeof(SettlementInfoConfirmField.ConfirmTime) - 1);
		m_pUserApi->ReqSettlementInfoConfirm(&SettlementInfoConfirmField, 0);
#endif

		_semaphore.signal();
	}

	// 下单应答
	void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("OnRspOrderInsert. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}
		printf("OnRspOrderInsert:InstrumentID:%s,ExchangeID:%s,VolumeTotalOriginal:%d,LimitPrice:%lf,RequestID:%d,InvestUnitID:%s\n", pInputOrder->InstrumentID, pInputOrder->ExchangeID, pInputOrder->VolumeTotalOriginal, pInputOrder->LimitPrice, pInputOrder->RequestID, pInputOrder->InvestUnitID);
	}

	// 撤单应答
	void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("OnRspOrderAction. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}
		printf("OnRspOrderAction:InstrumentID:%s,ExchangeID:%s,OrderSysID:%s,FrontID:%d,SessionID:%d,OrderRef:%s,RequestID:%d,InvestUnitID:%s\n",pInputOrderAction->InstrumentID,pInputOrderAction->ExchangeID,pInputOrderAction->OrderSysID, pInputOrderAction->FrontID, pInputOrderAction->SessionID, pInputOrderAction->OrderRef,pInputOrderAction->RequestID,pInputOrderAction->InvestUnitID);
	}


	// 查询品种
	void qryProduct()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		printf("查询品种 ...\n");
		CThostFtdcQryProductField QryProduct = { 0 };
		m_pUserApi->ReqQryProduct(&QryProduct, 0);
		_semaphore.wait();
	}

	/*产品查询应答*/
	void OnRspQryProduct(CThostFtdcProductField* pProduct, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pProduct)
			printf("OnRspQryProduct:ProductID:%s,ProductName:%s,ProductClass:%c,ExchangeID:%s\n", pProduct->ProductID, pProduct->ProductName, pProduct->ProductClass, pProduct->ExchangeID);

		if (bIsLast) {
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
			printf("OnRspQryExchange:ExchangeID:%s,ExchangeName:%s\n", pExchange->ExchangeID, pExchange->ExchangeName);

		if (bIsLast) {
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
			_semaphore.signal();
		}
	}

	// 查询交易所保证金率
	void qryExchangeMarginRate()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		printf("查询交易所保证金率 ...\n");
		CThostFtdcQryExchangeMarginRateField QryExchangeMarginRate = { 0 };
		strncpy(QryExchangeMarginRate.BrokerID, "8888", sizeof(QryExchangeMarginRate.BrokerID) - 1);
		//strncpy(QryExchangeMarginRate.InvestorID, user.c_str(), sizeof(QryExchangeMarginRate.InvestorID) - 1);
		strncpy(QryExchangeMarginRate.ExchangeID, "SSE", sizeof(QryExchangeMarginRate.ExchangeID) - 1);
		strncpy(QryExchangeMarginRate.InstrumentID, "10006150", sizeof(QryExchangeMarginRate.InstrumentID) - 1);
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
			_semaphore.signal();
		}
	}

	// 查询合约
	void qryInstrument()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		CThostFtdcQryInstrumentField QryInstrument = { 0 };
		m_pUserApi->ReqQryInstrument(&QryInstrument, 0);
		_semaphore.wait();
	}

	// 查询合约列表
	void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInstrument)
			printf("OnRspQryInstrument:InstrumentID:%s,InstrumentName:%s,ProductID:%s,PriceTick:%lf,UnderlyingInstrID:%s,StrikePrice:%lf,ExchangeID:%s\n", pInstrument->InstrumentID, pInstrument->InstrumentName, pInstrument->ProductID, pInstrument->PriceTick, pInstrument->UnderlyingInstrID, pInstrument->StrikePrice, pInstrument->ExchangeID);

		if (bIsLast) {
			_semaphore.signal();
		}
	}

	// 查询行情
	void qryDepthMarketdata()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		CThostFtdcQryDepthMarketDataField QryDepthMarketData = { 0 };
		strncpy(QryDepthMarketData.InstrumentID, "10006150", sizeof("10006150"));
		strncpy(QryDepthMarketData.ExchangeID, "SSE", sizeof("SSE"));
		m_pUserApi->ReqQryDepthMarketData(&QryDepthMarketData, 0);
		_semaphore.wait();
	}

	// 查询行情列表
	void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("OnRspQryDepthMarketData. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}

		if(pDepthMarketData)
			printf("OnRspQryDepthMarketData:InstrumentID:%s,LastPrice:%lf,Volume:%d,Turnover:%lf,OpenPrice:%lf,HighestPrice:%lf,LowestPrice:%lf,UpperLimitPrice:%lf,LowerLimitPrice:%lf,OpenInterest:%lf,PreClosePrice:%lf,PreSettlementPrice:%lf,SettlementPrice:%lf,UpdateTime:%s,ActionDay:%s,TradingDay:%s,ExchangeID:%s\n",
				pDepthMarketData->InstrumentID, double_format(pDepthMarketData->LastPrice), pDepthMarketData->Volume, double_format(pDepthMarketData->Turnover), double_format(pDepthMarketData->OpenPrice),
				double_format(pDepthMarketData->HighestPrice), double_format(pDepthMarketData->LowestPrice), double_format(pDepthMarketData->UpperLimitPrice), double_format(pDepthMarketData->LowerLimitPrice),
				double_format(pDepthMarketData->OpenInterest), double_format(pDepthMarketData->PreClosePrice), double_format(pDepthMarketData->PreSettlementPrice),
				double_format(pDepthMarketData->SettlementPrice), pDepthMarketData->UpdateTime, pDepthMarketData->ActionDay, pDepthMarketData->TradingDay, pDepthMarketData->ExchangeID);

		_lastPrice = double_format(pDepthMarketData->LastPrice);

		if (bIsLast) {
			_semaphore.signal();
		}
	}

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
			printf("OnRspQryOrder. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}

		if(pOrder)
			printf("OnRspQryOrder:BrokerID:%s,BrokerOrderSeq:%d,OrderLocalID:%s,InstrumentID:%s,Direction:%s,VolumeTotalOriginal:%d,LimitPrice:%lf,VolumeTraded:%d,VolumeTotal:%d,OrderSysID:%s,FrontID:%d,SessionID:%d,OrderRef:%s,OrderStatus:%c,StatusMsg:%s,ExchangeID:%s,InsertTime:%s,ClientID:%s,RequestID:%d,InvestUnitID:%s\n",
				pOrder->BrokerID,pOrder->BrokerOrderSeq,pOrder->OrderLocalID, pOrder->InstrumentID, direction_to_string(pOrder->Direction).c_str(), pOrder->VolumeTotalOriginal, pOrder->LimitPrice, pOrder->VolumeTraded, pOrder->VolumeTotal, pOrder->OrderSysID, pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef, pOrder->OrderStatus, pOrder->StatusMsg, pOrder->ExchangeID, pOrder->InsertTime,pOrder->ClientID,pOrder->RequestID,pOrder->InvestUnitID);

		if (bIsLast) {
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
			printf("OnRspQryTrade. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}

		if(pTrade)
			printf("OnRspQryTrade:BrokerID:%s,BrokerOrderSeq:%d,OrderLocalID:%s,InstrumentID:%s,Direction:%s,Volume:%d,Price:%lf,OrderSysID:%s,OrderRef:%s,ExchangeID:%s,TradeTime:%s,ClientID:%s,InvestUnitID:%s\n",
				pTrade->BrokerID, pTrade->BrokerOrderSeq, pTrade->OrderLocalID, pTrade->InstrumentID, direction_to_string(pTrade->Direction).c_str(), pTrade->Volume, pTrade->Price, pTrade->OrderSysID, pTrade->OrderRef, pTrade->ExchangeID, pTrade->TradeTime,pTrade->ClientID,pTrade->InvestUnitID);

		if (bIsLast) {
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
			printf("OnRspQryInvestorPosition. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}

		if (pInvestorPosition)
			printf("OnRspQryInvestorPosition:InstrumentID:%s,PosiDirection:%s,HedgeFlag:%c,Position:%d,YdPosition:%d,TodayPosition:%d,PositionCost:%lf,OpenCost:%lf,ExchangeID:%s\n",
				pInvestorPosition->InstrumentID, posidirection_to_string(pInvestorPosition->PosiDirection).c_str(), pInvestorPosition->HedgeFlag, pInvestorPosition->Position, pInvestorPosition->YdPosition, pInvestorPosition->TodayPosition, pInvestorPosition->PositionCost, pInvestorPosition->OpenCost, pInvestorPosition->ExchangeID);

		if (bIsLast) {
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
			printf("OnRspQryInvestorPositionDetail. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}

		if (pInvestorPositionDetail)
			printf("OnRspQryInvestorPositionDetail:InstrumentID:%s,Direction:%s,HedgeFlag:%c,Volume:%d,OpenDate:%s,OpenPrice:%lf,Margin:%lf,ExchangeID:%s\n",
				pInvestorPositionDetail->InstrumentID, direction_to_string(pInvestorPositionDetail->Direction).c_str(), pInvestorPositionDetail->HedgeFlag, pInvestorPositionDetail->Volume, pInvestorPositionDetail->OpenDate, pInvestorPositionDetail->OpenPrice, pInvestorPositionDetail->Margin, pInvestorPositionDetail->ExchangeID);

		if (bIsLast) {
			_semaphore.signal();
		}
	}

	// 查询资金
	void qryTradingAccount()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		CThostFtdcQryTradingAccountField QryTradingAccount = { 0 };
		m_pUserApi->ReqQryTradingAccount(&QryTradingAccount, 0);
		_semaphore.signal();
	}

	///请求查询资金账户响应
	void OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pTradingAccount)
			printf("OnRspQryTradingAccount:AccountID:%s,Available:%lf,FrozenCash:%lf,FrozenCommission:%lf\n",
				pTradingAccount->AccountID, pTradingAccount->Available, pTradingAccount->FrozenCash, pTradingAccount->FrozenCommission);

		if (bIsLast)
			printf("Query completed.\n");
	}


	// 委托回报
	void OnRtnOrder(CThostFtdcOrderField* pOrder)
	{
		printf("OnRtnOrder:BrokerID:%s,BrokerOrderSeq:%d,OrderLocalID:%s,InstrumentID:%s,Direction:%s,VolumeTotalOriginal:%d,LimitPrice:%lf,VolumeTraded:%d,VolumeTotal:%d,OrderSysID:%s,FrontID:%d,SessionID:%d,OrderRef:%s,OrderStatus:%c,StatusMsg:%s,ExchangeID:%s,InsertTime:%s,ClientID:%s,RequestID:%d,InvestUnitID:%s\n",
			pOrder->BrokerID, pOrder->BrokerOrderSeq, pOrder->OrderLocalID, pOrder->InstrumentID, direction_to_string(pOrder->Direction).c_str(), pOrder->VolumeTotalOriginal, pOrder->LimitPrice, pOrder->VolumeTraded, pOrder->VolumeTotal, pOrder->OrderSysID, pOrder->FrontID, pOrder->SessionID, pOrder->OrderRef, pOrder->OrderStatus, pOrder->StatusMsg, pOrder->ExchangeID, pOrder->InsertTime,pOrder->ClientID,pOrder->RequestID,pOrder->InvestUnitID);
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
			printf("OnErrRtnOrderInsert. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);

		if(pInputOrder)
			printf("OnErrRtnOrderInsert:OrderRef:%s,InstrumentID:%s,Direction:%s,VolumeTotalOriginal:%d,LimitPrice:%lf,ExchangeID:%s,RequestID:%d,InvestUnitID:%s\n",
				pInputOrder->OrderRef, pInputOrder->InstrumentID, direction_to_string(pInputOrder->Direction).c_str(), pInputOrder->VolumeTotalOriginal, pInputOrder->LimitPrice, pInputOrder->ExchangeID, pInputOrder->RequestID, pInputOrder->InvestUnitID);
	}

	// 撤单错误回报
	void OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0)
			printf("OnErrRtnOrderAction. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);

		if (pOrderAction)
			printf("OnErrRtnOrderAction:FrontID:%d,SessionID:%d,OrderRef:%s,InstrumentID:%s,OrderActionRef:%d,OrderSysID:%s,LimitPrice:%lf,ExchangeID:%s,RequestID:%d,InvestUnitID:%s\n",
				pOrderAction->FrontID, pOrderAction->SessionID, pOrderAction->OrderRef, pOrderAction->InstrumentID, pOrderAction->OrderActionRef, pOrderAction->OrderSysID, pOrderAction->LimitPrice, pOrderAction->ExchangeID, pOrderAction->RequestID, pOrderAction->InvestUnitID);
	}

	///查询投资者结算结果
	void ReqQrySettlementInfo()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		CThostFtdcQrySettlementInfoField QrySettlementInfo = { 0 };
		strncpy(QrySettlementInfo.BrokerID, "8888", sizeof(QrySettlementInfo.BrokerID) - 1);
		strncpy(QrySettlementInfo.InvestorID, "333306255", sizeof(QrySettlementInfo.InvestorID) - 1);
		strncpy(QrySettlementInfo.TradingDay, "20240129", sizeof(QrySettlementInfo.TradingDay) - 1);
		m_pUserApi->ReqQrySettlementInfo(&QrySettlementInfo, 0);
		_semaphore.wait();

	}

	///请求查询投资者结算结果响应
	void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
	{
		if (pRspInfo && pRspInfo->ErrorID != 0)
			printf("OnRspQrySettlementInfo. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);

		if (pSettlementInfo)
			printf("OnRspQrySettlementInfo:TradingDay:%s,SettlementID:%d,BrokerID:%s,InvestorID:%s\n",
				pSettlementInfo->TradingDay, pSettlementInfo->SettlementID, pSettlementInfo->BrokerID, pSettlementInfo->InvestorID);

		if (bIsLast)
			printf("Query completed.\n");
	}


public:
	std::string m_host;
	std::string m_broker;
	std::string m_user;
	std::string m_password;
	std::string m_appid;
	std::string m_authcode;
	unsigned int m_nOrderRef;
	double _lastPrice;

	CThostFtdcTraderApi* m_pUserApi;
};


void display_usage()
{
	printf("usage:ctpprint host broker user password appid authcode\n");
	printf("example:ctpprint tcp://180.168.146.187:10130 9999 000001 888888 simnow_client_test 0000000000000000\n");
}


int main(int argc, char* argv[])
{
	if (argc != 7) {
		display_usage();
		return -1;
	}

	std::string host(argv[1]), broker(argv[2]), user(argv[3]), password(argv[4]), appid(argv[5]), authcode(argv[6]);

	CApplication Spi(host, broker, user, password, appid, authcode);

	// 启动
	if (Spi.Run() < 0)
		return -1;

	// 等待登录完成
	_semaphore.wait();

	while(true)
	{
		std::cout << "Please pick an action\r\n";
		std::cout << "1. 查询资金	";
		std::cout << "2. 查询订单	";
		std::cout << "3. 查询成交	";
		std::cout << "4. 查询持仓	";
		std::cout << "5. 查询结算单	";
		std::cout << "6. 限价委托\r\n";
		std::cout << "7. 市价委托	";
		std::cout << "8. 撤单		";
		std::cout << "9. 净头寸交易	";
		std::cout << "0. 退出		";
		std::cout << "a. 查询持仓明细 ";
		std::cout << "b. 查询行情\r\n";
		std::cout << "c. 查询交易所	";
		std::cout << "d. 查询保证金率 ";
		std::cout << "e. 查询品种	";
		std::cout << "f. 查询合约	";
		std::cout << "g. 查询品种\r\n";

		char cmd;
		std::cin >> cmd;

		switch (cmd)
		{
		case '1':	//查询资金
			Spi.qryTradingAccount();
			break;
		case '2': 	//查询订单
			Spi.qryOrder();
			break;
		case '3': 	//查询成交
			Spi.qryTrade();
			break;
		case '4': //查询持仓
			Spi.qryInvestorPositionField();
			break;
		case '5': 
			Spi.ReqQrySettlementInfo();
			break;
		case '6': 
//			bSucc = trader->entrustLmt(false);
			break;
		case '7': 
			Spi.insertOrder();
			break;
		case '8': 
//			bSucc = trader->cancel();
			break;
		case '9':
//			bSucc = trader->entrustLmt(true);
			break;
		case '0': 
			break;
		case 'a':
			Spi.qryInvestorPositionDetail();
			break;
		case 'b':
			Spi.qryDepthMarketdata();
			break;
		case 'c':
			Spi.qryExchange();
			break;
		case 'd':
			Spi.qryExchangeMarginRate();
			break;
		case 'e':
			Spi.qryProduct();
			break;
		case 'f':
			Spi.qryInstrument();
			break;
		case 'g':
//			Spi.insertOrder();
			break;
		default:
			cmd = 'X';
			break;
		}

		if (cmd != '0' && cmd != 'X')
		{
//			_semaphore.wait();
		}
		else if(cmd == '0')
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


	// 查询订单
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	printf("查询订单 ...\n");
	CThostFtdcQryOrderField QryOrder = { 0 };
	Spi.m_pUserApi->ReqQryOrder(&QryOrder, 0);
	_semaphore.wait();

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
