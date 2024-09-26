﻿#include <stdio.h>
#include <string>
#include <locale>
#include <codecvt>
#include <thread>
#include <chrono>
#include "emt_trader_api.h"

using namespace EMT::API;
#define EMT_CLIENT_ID 99

class CApplication : public TraderSpi
{
public:
	CApplication(std::string host, std::string user, std::string password):
		m_host(host),
		m_user(user),
		m_password(password)
	{
		m_pUserApi = TraderApi::CreateTraderApi(EMT_CLIENT_ID, ".", EMT_LOG_LEVEL_WARNING);
		m_pUserApi->RegisterSpi(this);
		m_pUserApi->SubscribePublicTopic(EMT_TERT_QUICK);
	}

	~CApplication()
	{
	}

	const std::string to_gb2312(const char* info)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t> > conv;
		std::wstring_convert<std::codecvt_byname<wchar_t, char, mbstate_t>> convert(new std::codecvt_byname<wchar_t, char, mbstate_t>(".936"));

		std::string error_msg = convert.to_bytes(conv.from_bytes(info));

		return std::move(error_msg);
	}

	const std::string time_to_string(int64_t tm)
	{
		std::string d,t,dt;
		d = std::to_string(tm / 1000000000);
		tm %= 1000000000;
		tm /= 1000;
		t = std::to_string(tm/10000)+":"+std::to_string(tm%10000/100) + ":" + std::to_string(tm % 100);
		dt = d + " " + t;
		return std::move(dt);
	}
	int Run()
	{
		std::string ip = m_host.substr(0, m_host.find(':'));
		std::string port = m_host.substr(m_host.find(':')+1);
		m_session_id = m_pUserApi->Login(ip.c_str(), atol(port.c_str()), m_user.c_str(), m_password.c_str(), EMT_PROTOCOL_TCP);
		if (!m_session_id) {
			printf("Login failed.\n");
			OnError(m_pUserApi->GetApiLastError());
			return -1;
		}
		printf("Login succeeded.\n");

		int r = 0;

		// 查询股票合约
		printf("查询股票合约 ...\n");
		EMTQuerySecurityInfoReq QuerySecurityInfoReq;
		strncpy(QuerySecurityInfoReq.ticker, "600000", sizeof(QuerySecurityInfoReq) - 1);
		QuerySecurityInfoReq.market = EMT_MKT_SH_A;
		if ((r = m_pUserApi->QuerySecurityInfo(&QuerySecurityInfoReq, m_session_id, 0)) != 0) {
			OnError(m_pUserApi->GetApiLastError());
			return 0;
		}

		// 查询期权合约
		printf("查询期权合约 ...\n");
		EMTQueryOptionAuctionInfoReq QueryOptionAuctionInfoReq;
		memset(&QueryOptionAuctionInfoReq, 0x00, sizeof(QueryOptionAuctionInfoReq));
		QueryOptionAuctionInfoReq.market = EMT_MKT_INIT;
		if ((r = m_pUserApi->QueryOptionAuctionInfo(&QueryOptionAuctionInfoReq, m_session_id, 0)) != 0) {
			OnError(m_pUserApi->GetApiLastError());
			return 0;
		}

		return 0;
	}

	virtual void OnDisconnected(uint64_t session_id, int reason)
	{
		printf("Disconnected.\n");
	}

	///错误响应
	virtual void OnError(EMTRI* error_info)
	{
		if (!error_info)
			return;
		printf("Error: %d - %s\n",error_info->error_id, to_gb2312(error_info->error_msg).c_str());
	}

	virtual void OnQuerySecurityInfo(EMTQuerySecurityInfoRsp* security, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id)
	{
		if (error_info && error_info->error_id != 0)
		{
			OnError(error_info);
			return;
		}

		if (security)
			printf("ticker:%s,ticker_name:%s,market:%d,qty_unit:%d,highest_price:%lf,minimum_price:%lf\n", security->ticker, to_gb2312(security->ticker_name).c_str(), security->market, (int)security->qty_unit, security->highest_price, security->minimum_price);

		// 查询订单
		if (is_last) {
			//std::this_thread::sleep_for(std::chrono::seconds(5));
			printf("查询委托单 ...\n");
			EMTQueryOrderReq Req = { 0 };
			int r = m_pUserApi->QueryOrders(&Req, m_session_id, 0);
			if (r != 0) {
				OnError(m_pUserApi->GetApiLastError());
				return;
			}
		}
	}

	virtual void OnQueryOptionAuctionInfo(EMTQueryOptionAuctionInfoRsp* option_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id)
	{
		if (error_info && error_info->error_id != 0)
		{
			OnError(error_info);
		}

		if (option_info)
			printf("ticker:%s,symbol:%s,contract_id:%s,underlying_security_id:%s,call_or_put:%d,last_trade_date:%d,price_tick:%lf,contract_unit:%d\n", option_info->ticker, option_info->symbol, option_info->contract_id, option_info->underlying_security_id, option_info->call_or_put, option_info->last_trade_date, option_info->price_tick, (int)option_info->contract_unit);
		if (is_last) {
			// 查询订单
			//std::this_thread::sleep_for(std::chrono::seconds(5));
			printf("查询委托单 ...\n");
			EMTQueryOrderReq Req = { 0 };
			int r = m_pUserApi->QueryOrders(&Req, m_session_id, 0);
			if (r != 0) {
				OnError(m_pUserApi->GetApiLastError());
				return;
			}
		}
	}

	virtual void OnQueryOrder(EMTQueryOrderRsp* order_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id) 
	{
		if (error_info && error_info->error_id != 0)
		{
			OnError(error_info);
			return;
		}

		if(order_info)
			printf("order_xtp_id:%I64u,order_client_id:%u,ticker:%s,quantity:%I64d,price:%lf,qty_left:%I64d,order_status:%s,insert_time:%s\n", order_info->order_emt_id, order_info->order_client_id, order_info->ticker, order_info->quantity, order_info->price, order_info->qty_left, order_status_desc[order_info->order_status], time_to_string(order_info->insert_time).c_str());

		if (is_last) {
			printf("查询成交单 ...\n");
			EMTQueryTraderReq Req = { 0 };
			if (m_pUserApi->QueryTrades(&Req, m_session_id, 0)) {
				OnError(m_pUserApi->GetApiLastError());
				return;
			}
		}
	}

	virtual void OnQueryTrade(EMTQueryTradeRsp* trade_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_idt)
	{
		if (error_info && error_info->error_id != 0)
		{
			OnError(error_info);
			return;
		}
		if (trade_info)
			printf("order_emt_id:%I64u,ticker:%s,quantity:%I64d,price:%lf,exec_id:%s,trade_time:%s\n", trade_info->order_emt_id, trade_info->ticker, trade_info->quantity, trade_info->price, trade_info->exec_id, time_to_string(trade_info->trade_time).c_str());

		if (is_last) {
			printf("查询持仓 ...\n");
			if (m_pUserApi->QueryPosition(NULL, m_session_id, 0)) {
				OnError(m_pUserApi->GetApiLastError());
				return;
			}
		}
	}

	virtual void OnQueryPosition(EMTQueryStkPositionRsp* investor_position, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id)
	{
		if (error_info && error_info->error_id != 0)
		{
			OnError(error_info);
			return;
		}

		if (investor_position)
			printf("ticker:%s,total_qty:%I64d,sellable_qty:%I64d,avg_price:%lf,yesterday_position:%I64d\n", investor_position->ticker, investor_position->total_qty, investor_position->sellable_qty, investor_position->avg_price, investor_position->yesterday_position);
		if (is_last) {
			printf("查询资金 ...\n");
			if (m_pUserApi->QueryAsset(m_session_id, 0)) {
				OnError(m_pUserApi->GetApiLastError());
				return;
			}
		}
	}

	virtual void OnQueryAsset(EMTQueryAssetRsp* trading_account, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id)
	{
		if (error_info && error_info->error_id != 0)
		{
			OnError(error_info);
			return;
		}

		if (trading_account)
			printf("banlance:%lf,buying_power:%lf,withholding_amount:%lf\n", trading_account->banlance, trading_account->buying_power, trading_account->withholding_amount);
		if (is_last)
			printf("Query completed.\n");
	}

	const char* order_status_desc[EMT_ORDER_STATUS_UNKNOWN+1] = {"初始化","全部成交","部分成交","部分撤单","未成交","已撤单","已拒绝","未知订单状态"};

	// 报单通知
	virtual void OnOrderEvent(EMTOrderInfo* order_info, EMTRI* error_info, uint64_t session_id)
	{
		printf("order_emt_id:%I64u,order_client_id:%u,ticker:%s,quantity:%I64d,price:%lf,qty_left:%I64d,order_status:%s,insert_time:%s\n", order_info->order_emt_id, order_info->order_client_id, order_info->ticker, order_info->quantity, order_info->price, order_info->qty_left, order_status_desc[order_info->order_status], time_to_string(order_info->insert_time).c_str());
	}

	// 成交通知
	virtual void OnTradeEvent(EMTTradeReport* trade_info, uint64_t session_id)
	{
		printf("order_emt_id:%I64u,ticker:%s,quantity:%I64d,price:%lf,exec_id:%s,trade_time:%I64d\n", trade_info->order_emt_id, trade_info->ticker, trade_info->quantity, trade_info->price, trade_info->exec_id, trade_info->trade_time);
	}

	// 撤单错误应答
	virtual void OnCancelOrderError(EMTOrderCancelInfo* cancel_info, EMTRI* error_info, uint64_t session_id) {}

	virtual void OnQueryStructuredFund(EMTStructuredFundInfo* fund_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id) {}

	virtual void OnQueryFundTransfer(EMTFundTransferNotice* fund_transfer_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id) {}

	virtual void OnFundTransfer(EMTFundTransferNotice* fund_transfer_info, EMTRI* error_info, uint64_t session_id) {}

	virtual void OnQueryETF(EMTQueryETFBaseRsp* etf_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id) {}

	virtual void OnQueryETFBasket(EMTQueryETFComponentRsp* etf_component_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id) {}

	virtual void OnQueryIPOInfoList(EMTQueryIPOTickerRsp* ipo_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id) {}

	virtual void OnQueryIPOQuotaInfo(EMTQueryIPOQuotaRsp* quota_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id) {}

	virtual void OnCreditCashRepay(EMTCrdCashRepayRsp* cash_repay_info, EMTRI* error_info, uint64_t session_id) {}

	virtual void OnQueryCreditCashRepayInfo(EMTCrdCashRepayInfo* cash_repay_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id) {}

	virtual void OnQueryCreditFundInfo(EMTCrdFundInfo* fund_info, EMTRI* error_info, int request_id, uint64_t session_id) {}

	virtual void OnQueryCreditDebtInfo(EMTCrdDebtInfo* debt_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id) {}

	virtual void OnQueryCreditTickerDebtInfo(EMTCrdDebtStockInfo* debt_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id) {}

	virtual void OnQueryCreditAssetDebtInfo(double remain_amount, EMTRI* error_info, int request_id, uint64_t session_id) {}

	virtual void OnQueryCreditTickerAssignInfo(EMTClientQueryCrdPositionStkInfo* assign_info, EMTRI* error_info, int request_id, bool is_last, uint64_t session_id) {}

	virtual void OnQueryCreditExcessStock(EMTClientQueryCrdSurplusStkRspInfo* stock_info, EMTRI* error_info, int request_id, uint64_t session_id) {}


private:
	std::string m_host;
	std::string m_user;
	std::string m_password;
	uint64_t m_session_id;

	EMT::API::TraderApi* m_pUserApi;
};


void display_usage()
{
	printf("usage:emtprint host user password\n");
	printf("example:emtprint 61.152.230.41:19088 000001 888888\n");
}


int main(int argc, char* argv[])
{
	if (argc != 4) {
		display_usage();
		return -1;
	}

	std::string host = argv[1];
	std::string user = argv[2];
	std::string password = argv[3];

	CApplication Spi(host,user,password);

	// 启动
	if (Spi.Run() < 0)
		return -1;

	getchar();

	return 0;
}
