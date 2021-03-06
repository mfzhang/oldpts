//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <system.hpp>
#include <SysUtils.hpp>
#include <Inifiles.hpp>
#include "UTrdItf_GX.h"
#include "uPriceFunc.h"
#pragma package(smart_init)

static encode_dll_wrapper _encode_dll;
//---------------------------------------------------------------------------
TTrdItf_GX::TTrdItf_GX(const char *psetupfile,ADDLOG plogfunc)
:TTrdItfBase(psetupfile,plogfunc)
{
	ZeroMemory(&FGTJASet,sizeof(TGTJASet));
	FHandle = NULL;
	FOrderDate = "";
	FAccountType = "";
	FSrcList = new TStringList();
	FDstList = new TStringList();
	ZeroMemory(FEnPassword, 255);
	ZeroMemory(FCustid, 50);
	ZeroMemory(FBankCode,100);
	FExt1[0]=0;
}
//---------------------------------------------------------------------------
TTrdItf_GX::~TTrdItf_GX()
{
	this->CloseLink();

	delete FSrcList;
	delete FDstList;
}

//---------------------------------------------------------------------------
int TTrdItf_GX::Init()  //初始化
{
	FOrderDate = Date().FormatString("yyyymmdd");

	tagKCBPConnectOption stKCBPConnection;
	memset(&stKCBPConnection, 0 , sizeof(stKCBPConnection));

	strcpy(stKCBPConnection.szServerName, FGTJASet.ServerName);
	stKCBPConnection.nProtocal = 0;//
	strcpy(stKCBPConnection.szAddress, FGTJASet.ServerAddr);
	stKCBPConnection.nPort = FGTJASet.ServerPort;
	strcpy(stKCBPConnection.szSendQName, FGTJASet.SendQName/*"req1"*/);
	strcpy(stKCBPConnection.szReceiveQName, FGTJASet.ReceiveQName/*"ans1"*/);

	this->CloseLink();

	int nReturnCode;
	//新建KCBP实例
	if( (nReturnCode=KCBPCLI_Init( &FHandle )) != 0 )
	{
		FHandle=NULL;
		ODS('M',PLUGINNAME, "KCBPCLI_Init error,code=%d",nReturnCode );
		return -1;
	}
	//ODS('M',PLUGINNAME, "KCBPCLI_Init");

	//连接KCBP服务器
	if( (nReturnCode=KCBPCLI_SetConnectOption( FHandle, stKCBPConnection )) != 0)
	{
		KCBPCLI_Exit( FHandle );
		FHandle=NULL;
		ODS('M',PLUGINNAME, "KCBPCLI_SetConnectOption error,code=%d",nReturnCode );
		return -1;
	}
	//ODS('M',PLUGINNAME, "KCBPCLI_SetConnectOption");

	if( (nReturnCode=KCBPCLI_SetCliTimeOut( FHandle, FGTJASet.Timeout )) != 0)
	{
		KCBPCLI_Exit( FHandle );
		FHandle=NULL;
		ODS('M',PLUGINNAME, "KCBPCLI_SetCliTimeOut error,code=%d",nReturnCode );
		return -1;
	}
	//ODS('M',PLUGINNAME, "KCBPCLI_SetCliTimeOut");

	if((nReturnCode = KCBPCLI_ConnectServer( FHandle, stKCBPConnection.szServerName,
		FGTJASet.UserName, FGTJASet.Password)) !=0)
	{
		KCBPCLI_Exit( FHandle );
		FHandle=NULL;
		ODS('M',PLUGINNAME,"KCBPCLI_ConnectServer error,code=%d", nReturnCode);
		return -1;
	}
	//ODS('M',PLUGINNAME, "KCBPCLI_ConnectServer");



	return 0;
}
//---------------------------------------------------------------------------
int TTrdItf_GX::GetStkAccount(char *LogType, char *Logid, char *ZJ, char *SH, char *SZ)//获得主股东账号
{
	char szFunid[] = "410301";

	if(InitRequest(szFunid, true) != 0)
		return -1;

	KCBPCLI_SetValue(FHandle, "inputtype", LogType);//登录类型	inputtype	char(1)	Y	见备注
	KCBPCLI_SetValue(FHandle, "inputid", Logid);//登录标识	inputid	char(64)	Y	见备注

	int nReturnCode = ExecuteRequest(szFunid);
	if( nReturnCode != 0 && nReturnCode != 100 ) //nReturnCode = 0 全部返回了,=100表示还有数据
	{
		return nReturnCode;
	}
	char szTmpbuf[20];
	if( KCBPCLI_SQLMoreResults(FHandle) == 0 )
	{
		try
		{
			bool bFlag[2] = {false, false};
			while( !KCBPCLI_RsFetchRow(FHandle) )
			{
				if( KCBPCLI_RsGetColByName( FHandle, "market", szTmpbuf ) == 0)
				{
					if(lstrcmpi(szTmpbuf, "1") == 0 && !bFlag[0])
					{
						bFlag[0] = true;
						if( KCBPCLI_RsGetColByName( FHandle, "secuid", SH) != 0)
							return -1;
					}
					else if(lstrcmpi(szTmpbuf,this->FGTJASet.bencrypt==true ? "2":"0") == 0 && !bFlag[1])
					{
						bFlag[1] = true;
						if( KCBPCLI_RsGetColByName( FHandle, "secuid", SZ) != 0)
							return -1;
					}
				}
				else
					return -1;

				if(KCBPCLI_RsGetColByName( FHandle, "fundid", ZJ ) != 0)
					return -1;

				if(KCBPCLI_RsGetColByName( FHandle, "custid", FCustid ) != 0)
					return -1;

				if(KCBPCLI_RsGetColByName( FHandle, "bankcode", FBankCode ) != 0)
					return -1;

				//if(KCBPCLI_RsGetColByName( FHandle, "extprop", FExt1 ) != 0)
				//	return -1;

				//ODS('M',PLUGINNAME,"extprop:%s", FExt1);

				if(bFlag[0] && bFlag[1])
					break;
			}
		}
		__finally
		{
			KCBPCLI_SQLCloseCursor(FHandle);
		}
	}
	else
		return -1;

	return 0;
}
//---------------------------------------------------------------------------
int TTrdItf_GX::SetAccount() //设置帐号
{
	//ODS("---SetAccount");
	_encode_dll.lock();
	try
	{
		String sLogType = "Z", sLogid = FRequest.SetAccount.Account;
		char szSH[20] = {0x0, };
		char szSZ[20] = {0x0, };
		char szZJ[20] = {0x0, };

		FAccInfo.lt = FRequest.SetAccount.LoginType;
		lstrcpyn(FAccInfo.PW, FRequest.SetAccount.PW, PWD_LEN);//帐号密码

		memset(FEnPassword,0,255);
		//ZeroMemory(FEnPassword, 255);
		if(Encrypt(FRequest.SetAccount.PW, FEnPassword,/*sLogid.c_str()*/"410301") != 0)
			return -1;

		if(FRequest.SetAccount.LoginType!= asCA)
		{
			sLogType = "N";
			if(strlen(FRequest.SetAccount.Account) > 0 && !isdigit(FRequest.SetAccount.Account[0]))
				sLogid = FRequest.SetAccount.Account+1;
		}

		int ret =  GetStkAccount(sLogType.c_str(), sLogid.c_str(), szZJ, szSH, szSZ) ;
		if( ret != 0)
			return ret;

		if(FRequest.SetAccount.LoginType==asSHA)
		{
			FAccountType = "1";
			lstrcpyn(FAccInfo.SH, szSH, ACC_LEN);
			lstrcpyn(FAccInfo.SZ, szSZ, ACC_LEN);
			lstrcpyn(FAccInfo.Capital, szZJ, ACC_LEN);//资金帐号
		}
		else if(FRequest.SetAccount.LoginType==asSZA)
		{
			FAccountType = "2";
			lstrcpyn(FAccInfo.SZ, szSZ, ACC_LEN);
			lstrcpyn(FAccInfo.SH, szSH, ACC_LEN);
			lstrcpyn(FAccInfo.Capital, szZJ, ACC_LEN);//资金帐号
		}
		else if(FRequest.SetAccount.LoginType == asCA)//资金帐号
		{
			FAccountType = "";
			lstrcpyn(FAccInfo.SH, szSH, ACC_LEN);
			lstrcpyn(FAccInfo.SZ, szSZ, ACC_LEN);
			lstrcpyn(FAccInfo.Capital, szZJ, ACC_LEN);//资金帐号
		}
		else
		{
			ODS('M',PLUGINNAME,"无效的用户帐号类型:%c", FRequest.SetAccount.LoginType);
			return -1;
		}

		NewAns(1);
		memcpy(&((*FAnswer)[0].SetAccount.AccountInfo),&FAccInfo,sizeof(TAccountInfo));

		//ZeroMemory(FEnPassword, 255);
		memset(FEnPassword,0,255);
		Encrypt(FRequest.SetAccount.PW, FEnPassword, FCustid);

		//ODS("---SetAccount--end");
	}
	__finally
	{
		_encode_dll.unlock();
	}
	return 0;
}

//---------------------------------------------------------------------------
int TTrdItf_GX::QryMoney()     //资金查询
{
	char szFunid[] = "420502";
	if(InitRequest(szFunid) != 0)
		return -1;

	KCBPCLI_SetValue(FHandle, "fundid", this->FAccInfo.Capital);//资金账号
	KCBPCLI_SetValue(FHandle, "moneytype",
		ConvertMoneyType(FRequest.QryMoney.MoneyType)); //0	人民币

	int nReturnCode = ExecuteRequest(szFunid);
	if( nReturnCode != 0 && nReturnCode != 100 ) //nReturnCode = 0 全部返回了,=100表示还有数据
		return nReturnCode;

	char szTmpbuf[20];
	if( KCBPCLI_SQLMoreResults(FHandle) == 0 )
	{
		try
		{
			if( KCBPCLI_RsFetchRow(FHandle) == 0)
			{
				NewAns(1);
				if( KCBPCLI_RsGetColByName( FHandle, "fundbal", szTmpbuf ) == 0)
					(*FAnswer)[0].QryMoney.MoneyInfo.balance = StrToFloat(szTmpbuf);//资金余额
				else
					return -1;

				if( KCBPCLI_RsGetColByName( FHandle, "fundavl", szTmpbuf ) == 0)
					(*FAnswer)[0].QryMoney.MoneyInfo.available = StrToFloat(szTmpbuf);
				else
					return -1;

				if( KCBPCLI_RsGetColByName( FHandle, "moneytype", szTmpbuf ) == 0)
					(*FAnswer)[0].QryMoney.MoneyInfo.MoneyType = ConvertMoneyType(szTmpbuf);
				else
					return -1;
			}
			else
				return -1;
		}
		__finally
		{
			KCBPCLI_SQLCloseCursor(FHandle);
		}
	}
	else
		return -1;

	return 0;
}

//---------------------------------------------------------------------------
int TTrdItf_GX::QryCurCsn()    //当日委托查询
{
	char szFunid[] = "420510";

	String sPoststr = FRequest.QryCurCsn.SeqNum;
	int nReturnCode = -1;//-1表示第一次进入循环
	FSrcList->Clear();
	FDstList->Clear();
	const int nFieldCount = 15;
	FSrcList->Add("ordersno");
	FSrcList->Add("stkcode");
	FSrcList->Add("stkname");
	FSrcList->Add("market");
	FSrcList->Add("opertime");
	FSrcList->Add("orderdate");
	FSrcList->Add("bsflag");
	FSrcList->Add("orderstatus");
	FSrcList->Add("orderqty");
	FSrcList->Add("matchqty");
	FSrcList->Add("cancelqty");
	FSrcList->Add("orderprice");
	FSrcList->Add("matchamt");
	FSrcList->Add("secuid");
	FSrcList->Add("poststr");

	while(nReturnCode == -1 || nReturnCode == 100 || nReturnCode == 0)
	{
		if(InitRequest(szFunid) != 0)
			return -1;

		//qryflag实际应该一直送1，文档有误
		KCBPCLI_SetValue(FHandle, "market", "");//FAccountType.c_str());//交易市场		char(1)	N	不送查询全部
		KCBPCLI_SetValue(FHandle, "fundid", this->FAccInfo.Capital);//资金帐户		int	N
		KCBPCLI_SetValue(FHandle, "secuid", "");//sStkAcc.c_str());//股东代码		char(10)	N	不送查询全部
		KCBPCLI_SetValue(FHandle, "stkcode", "");//证券代码		char(8)	N	不送查询全部
		KCBPCLI_SetValue(FHandle, "ordersno", FRequest.QryCurCsn.OrderID);//委托序号	Ordersno	int	N	不送查询全部
		KCBPCLI_SetValue(FHandle, "bankcode",FBankCode);//外部银行	Bankcode	char(4)	N	三方交易时送
		KCBPCLI_SetValue(FHandle, "qryflag", "1");//(nReturnCode==-1?"1":"0"));//查询方向		char(1) 	Y	向下/向上查询方向,第一次查询送1
		KCBPCLI_SetValue(FHandle, "count", IntToStr(FRequest.QryCurCsn.MaxRecord).c_str());//请求行数		int 	Y	每次取的行数
		KCBPCLI_SetValue(FHandle, "poststr", sPoststr.c_str());//定位串  		char(32)	Y	第一次填空

		nReturnCode = ExecuteRequest(szFunid);//返回0也可能是多结果集，文档上没有提及啊
		if( nReturnCode != 0 && nReturnCode != 100 ) //nReturnCode = 0 全部返回了,=100表示还有数据
		{
			return nReturnCode;
		}
		int nReturnRow = QueryMoreResults(FSrcList, FDstList);
		if(nReturnRow == 0)
			break;
		else if(nReturnRow < 0)
		{
			ODS('M',PLUGINNAME," else if(nReturnRow < 0)");
			return -1;
		}
		else if(nReturnRow < FRequest.QryCurCsn.MaxRecord && nReturnCode != 100)//金正那边文档实在值得怀疑，还是安全一点吧
			break;


		if(FDstList->Count > 0)
			sPoststr = FDstList->Strings[FDstList->Count - 1];

		break;
	}

	if(FDstList->Count >= nFieldCount)
	{
		int nRows = FDstList->Count/nFieldCount;
		NewAns(nRows);

		//set value;
		for(int i=0; i<nRows; i++)
		{
			TOrderInfo *pConsInfo = &(*FAnswer)[i].QryCurCsn.OrderInfo;

			strcpy(pConsInfo->ContractID, FDstList->Strings[i*nFieldCount].Trim().c_str());//合同序号
			pConsInfo->Market = ConvertMarketType(FDstList->Strings[i*nFieldCount+3].Trim().c_str());
			lstrcpyn(pConsInfo->SecuID, FDstList->Strings[i*nFieldCount+1].Trim().c_str(), CODE_LEN);//证券代码
			pConsInfo->Time=ConvertTimeToInt(FDstList->Strings[i*nFieldCount+4].Trim().c_str());//委托时间
			pConsInfo->Date=ConvertDateToInt(FDstList->Strings[i*nFieldCount+5].Trim().c_str());//委托日期
			pConsInfo->Type = ConvertOrderType(FDstList->Strings[i*nFieldCount+6].Trim().c_str());//买卖类别
			pConsInfo->State = ConvertOrderState(FDstList->Strings[i*nFieldCount+7].Trim().c_str());///撤单状态---如何赋值
			pConsInfo->CsnVol = StrToInt(FDstList->Strings[i*nFieldCount+8].Trim());//委托数量
			pConsInfo->CancelVol = -StrToInt(FDstList->Strings[i*nFieldCount+10].Trim());//撤单数量=负值
			pConsInfo->CsnPrice = StrToFloat(FDstList->Strings[i*nFieldCount+11].Trim());//委托价格
			pConsInfo->TrdPrice = (pConsInfo->TrdVol >0 ? StrToFloat(FDstList->Strings[i*nFieldCount+12].Trim())/pConsInfo->TrdVol : 0);//成交价格---???结构体中没有相应字段
			lstrcpyn(pConsInfo->Account,FDstList->Strings[i*nFieldCount+13].Trim().c_str(),ACC_LEN);
			lstrcpyn(pConsInfo->SeqNum,FDstList->Strings[i*nFieldCount+14].Trim().c_str(),SEQ_LEN);
		}
	}

	return 0;
}

//---------------------------------------------------------------------------
int TTrdItf_GX::QryCurTrd()    //当日成交查询
{
	char szFunid[] = "420512";
	int machtype;
	String sPoststr = FRequest.QryCurTrd.SeqNum;
	int nReturnCode = -1;//-1表示第一次进入循环
	FSrcList->Clear();
	FDstList->Clear();
	const int nFieldCount = 13;

	FSrcList->Add("market");
	FSrcList->Add("stkcode");
	FSrcList->Add("stkname");
	FSrcList->Add("matchtime");
	FSrcList->Add("trddate");
	FSrcList->Add("bsflag");
	FSrcList->Add("matchqty");
	FSrcList->Add("matchprice");
	FSrcList->Add("matchtype");
	FSrcList->Add("matchcode");
	FSrcList->Add("ordersno");
	FSrcList->Add("poststr");
	FSrcList->Add("secuid");

	while(nReturnCode == -1 || nReturnCode == 100 || nReturnCode == 0)
	{
		if(InitRequest(szFunid) != 0)
			return -1;

		KCBPCLI_SetValue(FHandle, "fundid", FAccInfo.Capital);//资金帐户		int	N
		KCBPCLI_SetValue(FHandle, "market", "");//FAccountType.c_str());//交易市场		char(1)	N	不送查询全部
		KCBPCLI_SetValue(FHandle, "secuid", "");//sStkAcc.c_str());//股东代码		char(10)	N	不送查询全部
		KCBPCLI_SetValue(FHandle, "stkcode", "");//证券代码		char(8)	N	不送查询全部
		KCBPCLI_SetValue(FHandle, "ordersno", FRequest.QryCurTrd.OrderID);//委托序号	Ordersno	int	N	不送查询全部
		KCBPCLI_SetValue(FHandle, "bankcode", FBankCode);//外部银行	Bankcode	char(4)	N	三方交易时送
		KCBPCLI_SetValue(FHandle, "qryflag", "1");//(nReturnCode==-1?"1":"0"));//查询方向		char(1) 	Y	向下/向上查询方向,第一次查询送1
		KCBPCLI_SetValue(FHandle, "count", IntToStr(FRequest.QryCurTrd.MaxRecord).c_str());//请求行数		int 	Y	每次取的行数
		KCBPCLI_SetValue(FHandle, "poststr", sPoststr.c_str());//定位串  		char(32)	Y	第一次填空

		nReturnCode = ExecuteRequest(szFunid);
		if( nReturnCode != 0 && nReturnCode != 100 ) //nReturnCode = 0 全部返回了,=100表示还有数据
			return nReturnCode;

		int nReturnRow = QueryMoreResults(FSrcList, FDstList);
		if(nReturnRow == 0)
			break;
		else if(nReturnRow < 0)
			return -1;
		else if(nReturnRow < FRequest.QryCurTrd.MaxRecord && nReturnCode != 100)//金正那边文档实在值得怀疑，还是安全一点吧
			break;


		if(FDstList->Count > 0)
			sPoststr = FDstList->Strings[FDstList->Count - 1];

		break;
	}

	if(FDstList->Count >= nFieldCount)
	{
		int nRows = FDstList->Count/nFieldCount;
		NewAns(nRows);

		//set value;
		for(int i=0; i<nRows; i++)
		{
			TTradeInfo *pTrdInfo = &((*FAnswer)[i].QryCurTrd.TrdInfo);
			lstrcpyn(pTrdInfo->ContractID,FDstList->Strings[i*nFieldCount+10].Trim().c_str(),SEQ_LEN);
			lstrcpyn(pTrdInfo->TrdID,FDstList->Strings[i*nFieldCount+9].Trim().c_str(),SEQ_LEN);
			pTrdInfo->Market = ConvertMarketType(FDstList->Strings[i*nFieldCount].Trim().c_str());
			lstrcpyn(pTrdInfo->SecuID, FDstList->Strings[i*nFieldCount+1].Trim().c_str(), CODE_LEN);//证券代码
			pTrdInfo->Time = ConvertTimeToInt(FDstList->Strings[i*nFieldCount+3].Trim().c_str());//成交时间
			pTrdInfo->Date = ConvertDateToInt(FDstList->Strings[i*nFieldCount+4].Trim().c_str());//委托日期
			pTrdInfo->Type = ConvertOrderType(FDstList->Strings[i*nFieldCount+5].Trim().c_str());//买卖类别
			pTrdInfo->Vol = StrToInt(FDstList->Strings[i*nFieldCount+6].Trim());//成交数量
			pTrdInfo->Price = StrToFloat(FDstList->Strings[i*nFieldCount+7].Trim());//成交价格
			machtype = StrToIntDef(FDstList->Strings[i*nFieldCount+8].Trim(),-1) ;
			if( machtype != 0 )
				pTrdInfo->Vol = -pTrdInfo->Vol;
			lstrcpyn(pTrdInfo->SeqNum,FDstList->Strings[i*nFieldCount+11].Trim().c_str(),SEQ_LEN);
			lstrcpyn(pTrdInfo->Account,FDstList->Strings[i*nFieldCount+12].Trim().c_str(),ACC_LEN);
		}
	}

	return 0;
}
//---------------------------------------------------------------------------
int TTrdItf_GX::QryStocks()    //查询帐号中所有的股份
{
	char szFunid[] = "420503";

	String sPoststr = "";
	int nReturnCode = -1;//-1表示第一次进入循环
	FSrcList->Clear();
	FDstList->Clear();
	const int nFieldCount = 13;
	FSrcList->Add("market");
	FSrcList->Add("stkcode");
	FSrcList->Add("stkname");
	FSrcList->Add("stkbal");
	FSrcList->Add("stkavl");
	FSrcList->Add("buycost");
	FSrcList->Add("costprice");
	FSrcList->Add("mktval");
	FSrcList->Add("lastprice");
	FSrcList->Add("income");
	FSrcList->Add("secuid");
	FSrcList->Add("stkqty");
	FSrcList->Add("poststr");

	String sStkAcc = this->GetStockAccountByMarket(FRequest.QryStocks.Market);
	String sMarket = ConvertMarketType(FRequest.QryStocks.Market);

	while(nReturnCode == -1 || nReturnCode == 100 || nReturnCode == 0)
	{
		if(InitRequest(szFunid) != 0)
			return -1;

		KCBPCLI_SetValue(FHandle, "market", sMarket.c_str());//FAccountType.c_str());//交易市场	market	char(1)	N	不送查询全部
		KCBPCLI_SetValue(FHandle, "fundid", FAccInfo.Capital);//资金帐户		int	N
		KCBPCLI_SetValue(FHandle, "secuid", "");//sStkAcc.c_str());//股东代码		char(10)	N	不送查询全部
		KCBPCLI_SetValue(FHandle, "stkcode", "");//证券代码		char(8)	N	不送查询全部
		KCBPCLI_SetValue(FHandle, "qryflag", "1");//(nReturnCode==-1?"1":"0"));//查询方向		char(1) 	Y	向下/向上查询方向,第一次查询送1
		KCBPCLI_SetValue(FHandle, "count", GTJA_MAX_REC_NUM);//请求行数		int 	Y	每次取的行数
		KCBPCLI_SetValue(FHandle, "poststr", sPoststr.c_str());//定位串  		char(32)	Y	第一次填空

		nReturnCode = ExecuteRequest(szFunid);
		if( nReturnCode != 0 && nReturnCode != 100 ) //nReturnCode = 0 全部返回了,=100表示还有数据
			return nReturnCode;

		int nReturnRow = QueryMoreResults(FSrcList, FDstList);
		if(nReturnRow == 0)
			break;
		else if(nReturnRow < 0)
			return -1;
		else if(nReturnRow < GTJA_MAX_REC_NUM_INT && nReturnCode != 100)//金正那边文档实在值得怀疑，还是安全一点吧
			break;


		if(FDstList->Count > 0)
			sPoststr = FDstList->Strings[FDstList->Count - 1];

	}

	if(FDstList->Count >= nFieldCount)
	{
		int nRows = FDstList->Count/nFieldCount;
		NewAns(nRows);
		//set value;
		for(int i=0; i<nRows; i++)
		{
			TStockInfo *pStock = &(*FAnswer)[i].QryStocks.StockInfo;

			pStock->PosDir = pdNet;
			pStock->Market = ConvertMarketType(FDstList->Strings[i*nFieldCount].Trim().c_str());
			lstrcpyn(pStock->SecuID, FDstList->Strings[i*nFieldCount+1].Trim().c_str(), CODE_LEN);//证券代码
			lstrcpyn(pStock->Account,FDstList->Strings[i*nFieldCount+10].Trim().c_str(),ACC_LEN);

			pStock->PosNum = StrToIntDef(FDstList->Strings[i*nFieldCount+11].Trim(), 0);
			pStock->Balance = StrToIntDef(FDstList->Strings[i*nFieldCount+3].Trim(), 0);//余额
			pStock->Available = StrToIntDef(FDstList->Strings[i*nFieldCount+4].Trim(), 0);//可用数
			pStock->CostPrice = StrToFloatDef(FDstList->Strings[i*nFieldCount+6].Trim(),0);//参考成本价
			pStock->CurPrice = StrToFloatDef(FDstList->Strings[i*nFieldCount+8].Trim(),0);//参考市价
		}  
	}

	return 0;
}
//---------------------------------------------------------------------------
int TTrdItf_GX::Undo() //委托撤单
{
	char szFunid[] = "420413";
	if(InitRequest(szFunid) != 0)
		return -1;

	KCBPCLI_SetValue(FHandle, "orderdate", FOrderDate.c_str());//委托日期	orderdate	int	Y
	KCBPCLI_SetValue(FHandle, "fundid", FAccInfo.Capital);//资金账户	fundid	int	Y
	KCBPCLI_SetValue(FHandle, "ordersno", FRequest.Undo.ContractID);//委托序号	ordersno	int	Y	委托返回的

	int nReturnCode = ExecuteRequest(szFunid);
	if( nReturnCode != 0)
	{
		ODS('M',PLUGINNAME,"Undo ExecuteRequest=%d", nReturnCode);
		if( -410413020 ==  nReturnCode )
			return ERR_UNDO_CANNOT_CANCLE;
		else
			return nReturnCode;
	}

	char szTmpbuf[20];
	if( KCBPCLI_SQLMoreResults(FHandle) == 0 )
	{
		try
		{
			if( KCBPCLI_RsFetchRow(FHandle) != 0)
				return -1;
		}
		__finally
		{
			KCBPCLI_SQLCloseCursor(FHandle);
		}
	}
	else
		return -1;

	return 0;
}
//---------------------------------------------------------------------------

int  TTrdItf_GX::Order()
{
	//注：如果用资金账号登陆则要获取其上证及深证股东账号，否则买卖股票可能出错！
	char szFunid[] = "420411";
	if(InitRequest(szFunid) != 0)
		return -1;

	KCBPCLI_SetValue(FHandle, "market", ConvertMarketType(FRequest.Order.Market));
	KCBPCLI_SetValue(FHandle, "secuid", this->GetStockAccountByMarket(FRequest.Order.Market));

	String sPrice;
	if( Pos(AnsiString("510"),FRequest.Order.SecuID ) == 1 ||
		Pos(AnsiString("1599"),FRequest.Order.SecuID ) == 1 )
	{
		sPrice  = ConvetDoubleToPrice( FRequest.Order.Price,3);
		//ODS("%s,%s",FRequest.Order.SecuID,sPrice.c_str());
	}
	else
	{
		sPrice  = ConvetDoubleToPrice( FRequest.Order.Price,2);
		//ODS("%s,%s",FRequest.Order.SecuID,sPrice.c_str());
	}

	KCBPCLI_SetValue(FHandle, "fundid", FAccInfo.Capital);//资金账户	fundid	int	Y
	KCBPCLI_SetValue(FHandle, "stkcode", FRequest.Order.SecuID);//证券代码	stkcode	char(8)	Y
	KCBPCLI_SetValue(FHandle, "bsflag", ConvertOrderType(FRequest.Order.Type));//买卖类别	bsflag	char(1)	Y	B:买入S:卖出  3基金申购 4基金赎回
	KCBPCLI_SetValue(FHandle, "price", sPrice.c_str());//价格	price	numeric(9,3)	Y
	KCBPCLI_SetValue(FHandle, "qty", IntToStr(FRequest.Order.Num).c_str());//数量	qty	int	Y
	KCBPCLI_SetValue(FHandle, "ordergroup", "-1"/*"0"*/);//委托批号	ordergroup	int	Y	0,
	KCBPCLI_SetValue(FHandle, "bankcode",FBankCode);//外部银行	bankcode	char(4)	N	三方交易时送
	KCBPCLI_SetValue(FHandle, "remark", "");//备注信息	remark	char(64)	N

	int nReturnCode = ExecuteRequest(szFunid);
	if( nReturnCode != 0)
	{
		return nReturnCode;
	}

	char szTmpbuf[20];
	if( KCBPCLI_SQLMoreResults(FHandle) == 0 )
	{
		try
		{
			if( KCBPCLI_RsFetchRow(FHandle) == 0)
			{
				NewAns(1);

				if( KCBPCLI_RsGetColByName( FHandle, "ordersno", szTmpbuf ) == 0) //委托序号	ordersno	int	返回给股民
					lstrcpyn((*FAnswer)[0].Order.ContractID ,szTmpbuf,SEQ_LEN);
				else
					return -1;
			}
			else
				return -1;
		}
		__finally
		{
			KCBPCLI_SQLCloseCursor(FHandle);
		}
	}
	else
		return -1;

	return 0;
}
//---------------------------------------------------------------------------

int TTrdItf_GX::SetSytemParams(char *Funid, bool IsLogin)
{
	int nReturnCode = KCBPCLI_SetSystemParam(FHandle, KCBP_PARAM_RESERVED, FGTJASet.DptCode);//必须设置营业部代码
	if(nReturnCode != 0)   
	{
		ODS('M',PLUGINNAME,"KCBPCLI_SetSystemParam ErrCode=%d", nReturnCode);
		return -1;
	}

	if(!IsLogin)
	{
		KCBPCLI_SetValue(FHandle, "funcid", Funid);//功能号, 必须送,不可以为空
		KCBPCLI_SetValue(FHandle, "custid", FCustid);//客户代码,  可以为空119353,517486
		KCBPCLI_SetValue(FHandle, "custorgid", FGTJASet.DptCode);//客户机构, 可以为空
		//KCBPCLI_SetValue(FHandle, "trdpwd", FEnPassword);//交易密码, 可以为空
		KCBPCLI_SetValue(FHandle, "netaddr", FGTJASet.MAC.c_str());//FGTJASet.ServerAddr);//操作站点, 必须送，不可以为空
		KCBPCLI_SetValue(FHandle, "orgid", FGTJASet.DptCode);//操作机构, 必须送，不可以为空
		KCBPCLI_SetValue(FHandle, "operway", String(FGTJASet.Operway).c_str());//操作方式, 必须送，不可以为空
		KCBPCLI_SetValue(FHandle, "ext", "");//扩展字段, 必须送，可以为空
		KCBPCLI_SetValue(FHandle, "request", "");
		KCBPCLI_SetValue(FHandle, "ext1", FExt1);//扩展字段, 必须送，可以为空
		KCBPCLI_SetValue(FHandle, "inputtype", "C");
		KCBPCLI_SetValue(FHandle, "inputid", FCustid);
	}
	else
	{
		KCBPCLI_SetValue(FHandle, "funcid", Funid);//功能号, 必须送,不可以为空
		KCBPCLI_SetValue(FHandle, "custid", "");//客户代码,  可以为空 28014444,8237964
		KCBPCLI_SetValue(FHandle, "custorgid", "");//客户机构, 可以为空
		KCBPCLI_SetValue(FHandle, "trdpwd", FEnPassword);//交易密码, 可以为空
		KCBPCLI_SetValue(FHandle, "netaddr", FGTJASet.MAC.c_str());//FGTJASet.ServerAddr);//操作站点, 必须送，不可以为空
		KCBPCLI_SetValue(FHandle, "orgid", FGTJASet.DptCode);//操作机构, 必须送，不可以为空
		KCBPCLI_SetValue(FHandle, "operway", String(FGTJASet.Operway).c_str()/*"0"*/);//操作方式, 必须送，不可以为空
		KCBPCLI_SetValue(FHandle, "ext", "");//扩展字段, 必须送，可以为空
	}

	return 0;
}
//---------------------------------------------------------------------------
int TTrdItf_GX::CheckSqlExecute(void)
{
	int nErrCode = 0;
	char szErrMsg[256];

	if(KCBPCLI_GetErr(FHandle, &nErrCode, szErrMsg) == 0)
	{
		if(nErrCode != 0)
		{
			szErrMsg[255] = '\0';
			ODS('M',PLUGINNAME,"CheckSqlExecute,ErrCode:%d, ErrMsg:%s", nErrCode, szErrMsg );
			lstrcpyn(this->FLastError.Sender,"GTJAJZX",SEQ_LEN);
			this->FLastError.ErrorCode = nErrCode;
			lstrcpyn(this->FLastError.Text,szErrMsg,MAX_TEXT);
			return nErrCode;
		}
	}

	return 0;
}
//---------------------------------------------------------------------------
int TTrdItf_GX::CheckRsOpen(void)
{
	int nCol = 0;
	KCBPCLI_SQLNumResultCols(FHandle, &nCol); ///////////////
	if(KCBPCLI_SQLFetch(FHandle) != 0)
	{
		ODS('M',PLUGINNAME, "KCBPCLI_SQLFetch error!" );
		return -1;
	}

	char szTmpbuf[1024];
	if( KCBPCLI_RsGetColByName( FHandle, "CODE", szTmpbuf ) != 0)
	{
		ODS('M',PLUGINNAME, "Get CODE Fail\n" );
		return -1;
	}

	if( strcmp(szTmpbuf, "0") != 0 )
	{
		String sErrMsg = "";
		sErrMsg.cat_printf( "CheckRsOpen,error code :%s ", szTmpbuf );
		KCBPCLI_RsGetColByName( FHandle, "LEVEL", szTmpbuf );
		sErrMsg.cat_printf( "error level :%s ", szTmpbuf );
		KCBPCLI_RsGetColByName( FHandle, "MSG", szTmpbuf );
		sErrMsg.cat_printf( "error msg :%s\n", szTmpbuf );

		ODS('M',PLUGINNAME,sErrMsg.c_str());
		lstrcpyn(this->FLastError.Sender,"GTJAJZX",SEQ_LEN);
		this->FLastError.ErrorCode = atoi(szTmpbuf);
		lstrcpyn(this->FLastError.Text,sErrMsg.c_str(),MAX_TEXT);
		return this->FLastError.ErrorCode;
	}

	return 0;
}
//---------------------------------------------------------------------------
int TTrdItf_GX::InitRequest(char *Funid, bool IsLogin)
{
	/*初始化传入参数缓冲区*/
	int nReturnCode = KCBPCLI_BeginWrite(FHandle);
	if( nReturnCode !=0 ) 
	{
		ODS('M',PLUGINNAME,"KCBPCLI_BeginWrite ErrCode=%d", nReturnCode);
		return -1;
	}

	if(SetSytemParams(Funid, IsLogin) != 0)
		return -1;

	return 0;
}
//---------------------------------------------------------------------------
int TTrdItf_GX::ExecuteRequest(char *Funid)
{
	/*客户端向服务器提交请求*/
	int nReturnCode  = KCBPCLI_SQLExecute(FHandle, Funid);//

	if(nReturnCode>2000)
	{
		ODS('E',PLUGINNAME,"KCBPCLI_SQLExecute=%d",nReturnCode);
		return ERR_REMOTE_OTHER_FAILED;
	}

	if( CheckSqlExecute()!=0 )
	{
		return ERR_REMOTE_OTHER_FAILED;
	}

	nReturnCode = KCBPCLI_RsOpen(FHandle);
	if( nReturnCode != 0 && nReturnCode != 100 ) //nReturnCode = 0 全部返回了,=100表示还有数据
	{
		ODS('M',PLUGINNAME,"KCBPCLI_RsOpen ErrCode=%d", nReturnCode);
		return ERR_REMOTE_OTHER_FAILED;
	}

	return  CheckRsOpen() ;
}
//---------------------------------------------------------------------------
int TTrdItf_GX::QueryMoreResults(TStrings *pSrcList, TStrings *pDstList)
{
	int nRows = 0, nCol = 0;;
	char szTmpbuf[100] = {0x0, };
	if( KCBPCLI_SQLMoreResults(FHandle) == 0 )
	{
		try
		{
			KCBPCLI_SQLNumResultCols(FHandle, &nCol);
			while( !KCBPCLI_RsFetchRow(FHandle) )
			{
				for(int i=0; i<pSrcList->Count; i++ )
				{
					if(KCBPCLI_RsGetColByName( FHandle, pSrcList->Strings[i].c_str(), szTmpbuf ) == 0)
						pDstList->Add(szTmpbuf);
					else
					{
						ODS('M',PLUGINNAME,"KCBPCLI_RsGetColByName(%s) != 0", pSrcList->Strings[i].c_str());
						return -1;
					}
				}
				nRows++;
			}
		}
		__finally
		{
			KCBPCLI_SQLCloseCursor(FHandle);
		}
	}

	//ODS('M',PLUGINNAME,"%s", pDstList->Text.c_str());
	return nRows;
}
//---------------------------------------------------------------------------
int TTrdItf_GX::Encrypt(char *pSrc, char *pDst, char *key)
{
	return _encode_dll.Encrypt(pSrc,pDst,key,FGTJASet.bencrypt);
}
//---------------------------------------------------------------------------

void TTrdItf_GX::CloseLink(void)
{
	lock();
	if(FHandle != NULL)
	{
		try
		{
			KCBPCLI_DisConnect( FHandle);
			KCBPCLI_Exit( FHandle );
			ODS('M',PLUGINNAME, "KCBPCLI_Exit");
		}
		catch(Exception &e)
		{
			ODS('M',PLUGINNAME,"CloseLink error,%s",e.Message);
		}
		FHandle = NULL;
	}
	unlock();
	TTrdItfBase::CloseLink();
	//ODS('M',PLUGINNAME,"CloseLink");
}

int  TTrdItf_GX::KeepAlive(void)
{
	return 0;
}

market_type  TTrdItf_GX::ConvertMarketType(const char *t)
{
	if(t==NULL || strlen(t)<1 ) return mtNo;

	if( FGTJASet.SHA == t ) return mtSHA;
	else if( FGTJASet.SZA == t) return mtSZA;
	return mtNo;
}

order_type   TTrdItf_GX::ConvertOrderType(const char *t)
{
	if(t==NULL || strlen(t)<2 ) return otNo;
	switch (t[1]) {
		case 'B': return otBuy;
		case 'S': return otSell;
		case '1': return otETFSub;
		case '2': return otETFRed;
		default:
			return otNo;
	}
}

/*
委托状态orderstatus
代码	状态
0	未报
1	正报
2	已报
3	已报待撤
4	部成待撤
5	部撤
6	已撤
7	部成
8	已成
9	废单
A	待报
B	正报
*/
order_state  TTrdItf_GX::ConvertOrderState(const char *t)
{
	//'0','未报' 在lbm中委托成功写入未发标志
	//'1','正报' 在落地方式中,报盘扫描未发委托,并将委托置为正报状态
	//'2','已报' 报盘成功后,报盘机回写发送标志为已报
	//'3','已报待撤' 已报委托撤单
	//'4','部成待撤' 部分成交后委托撤单
	//'5','部撤' 部分成交后撤单成交
	//'6','已撤' 全部撤单成交
	//'7','部成' 已报委托部分成交
	//'8','已成' 委托或撤单全部成交
	//'9','废单' 委托确认为费单
	//'A','待报'
	//'B','正报' 国泰模式中，已报道接口库尚未处理返回库的阶段，标准模式不用。
	if(t==NULL || strlen(t)<1 ) return osno;
	switch(t[0])
	{
	case '0':
	case '1':
	case 'A':
	case 'B':
		return oswb;
	case '2':
		return osyb;
	case '3':
	case '4':
		return osdc;
	case '5':
		return osbw;
	case '6':
		return osyc;
	case '7':
		return osbc;
	case '8':
		return oscj;
	case '9':
		return osyf;
	default:
		return osno;
	}
}

pos_direction TTrdItf_GX::ConvertPosDir(const char *t)
{
	return pdNet;
}

money_type   TTrdItf_GX::ConvertMoneyType(const char *t)
{
	if(t==NULL || strlen(t)<1 ) return motNo;
	switch (t[0]) {
		case '0':return motRMB;
		case '2':return motUSD;
		case '1':return motHKD;
		default:
			return motNo;
	}
}

char *  TTrdItf_GX::ConvertMarketType(market_type t)
{
	switch (t) {
		case mtSHA : return FGTJASet.SHA.c_str();
		case mtSZA : return FGTJASet.SZA.c_str();
		default:
			return "";
	}
}

/*
外围委托买卖类别bsflag
代码	货币名称
0B	买入
0S	卖出
0G	转股
0H	回售
01	ETF申购
02	ETF赎回
03	LOF申购
04	LOF赎回
07	认沽行权
08	认售行权
0a	对方买入
0b	本方买入
0c	即时买入
0d	五档买入
0e	全额买入
0f	对方卖出
0g	本方卖出
0h	即时卖出
0i	五档卖出
0j	全额卖出
0n	入质
0p	出质
0q	转限买入
0r	转限卖出

*/
char *  TTrdItf_GX::ConvertOrderType(order_type t)
{
	switch (t) {
	case 	otBuy      : return "0B";
	case 	otSell     : return "0S";
	case 	otETFSub   : return "01";
	case 	otETFRed   : return "02";
	default:
		return "";
	}
}

char *  TTrdItf_GX::ConvertOrderState(order_state t)
{
	return "";
}

char *  TTrdItf_GX::ConvertPosDir(pos_direction t)
{
	return "";
}
/*
币moneytype
代码	货币名称
0	人民币
1	港币
2	美元

*/
char *  TTrdItf_GX::ConvertMoneyType(money_type t)
{
	switch (t) {
	case motRMB: return "0";
	case motUSD: return "2";
	case motHKD: return "1";
	default:
		return "";
	}
}

bool TTrdItf_GX::GetConfig(void)
{
	if( FSetupFile== NULL ) return false;

	TMemIniFile *ini = new TMemIniFile( FSetupFile );
	try
	{
		// 判断是否存在插件的设置，没有的话返回false以便系统能够生成默认的设置
		if( ini->SectionExists(PLUGINNAME) == false ) return false;
		lstrcpyn(FGTJASet.ServerName,
			ini->ReadString(PLUGINNAME, "ServerName", "RDC-ZHANGZ").c_str(),
			KCBP_SERVERNAME_MAX);
		lstrcpyn(FGTJASet.ServerAddr,
			ini->ReadString(PLUGINNAME, "ServerAddr", "127.0.0.1").c_str(),
			KCBP_DESCRIPTION_MAX);
		FGTJASet.ServerPort  = ini->ReadInteger(PLUGINNAME, "ServerPort", 21000);
		lstrcpyn(FGTJASet.DptCode,
			ini->ReadString(PLUGINNAME, "DptCode", "3104").c_str(),
			20);
		FGTJASet.Timeout  = ini->ReadInteger(PLUGINNAME, "Timeout", 20);
		lstrcpyn(FGTJASet.UserName ,
			ini->ReadString(PLUGINNAME, "UserName", "KCXP00").c_str(),
			20);
		lstrcpyn(FGTJASet.Password ,
			ini->ReadString(PLUGINNAME, "Password", "888888").c_str(),
			20);
		String tmp = ini->ReadString(PLUGINNAME, "Operway", "0");
		FGTJASet.Operway = tmp.Length()>0 ? tmp[1] : '0';
		lstrcpyn(FGTJASet.SendQName  ,
			ini->ReadString(PLUGINNAME, "SendQName", "req1").c_str(),
			20);
		lstrcpyn(FGTJASet.ReceiveQName ,
			ini->ReadString(PLUGINNAME, "ReceiveQName", "ans1").c_str(),
			20);
		FGTJASet.bencrypt = ini->ReadBool(PLUGINNAME,"Encrypt",true);
		FGTJASet.SHA = ini->ReadString(PLUGINNAME,"SHA","1");
		FGTJASet.SZA = ini->ReadString(PLUGINNAME,"SZA","0");
		FGTJASet.MAC = ini->ReadString(PLUGINNAME,"MAC","");
	}
	__finally
	{
		delete ini;
	}
	return true;
}

void TTrdItf_GX::SetConfig(void)
{
	if( FSetupFile== NULL ) return ;

	TMemIniFile *ini = new TMemIniFile( FSetupFile );
	try
	{
		ini->WriteString(PLUGINNAME, "ServerName", FGTJASet.ServerName);
		ini->WriteString(PLUGINNAME, "ServerAddr", FGTJASet.ServerAddr);
		ini->WriteInteger(PLUGINNAME, "ServerPort", FGTJASet.ServerPort);
		ini->WriteString(PLUGINNAME, "DptCode", FGTJASet.DptCode);
		ini->WriteInteger(PLUGINNAME, "Timeout", FGTJASet.Timeout);
		ini->WriteString(PLUGINNAME, "UserName", FGTJASet.UserName);
		ini->WriteString(PLUGINNAME, "Password", FGTJASet.Password);
		ini->WriteString(PLUGINNAME, "Operway", String(FGTJASet.Operway));
		ini->WriteString(PLUGINNAME, "SendQName", FGTJASet.SendQName);
		ini->WriteString(PLUGINNAME, "ReceiveQName", FGTJASet.ReceiveQName);
		ini->WriteBool(PLUGINNAME,"Encrypt",FGTJASet.bencrypt);
		ini->WriteString(PLUGINNAME,"SHA",FGTJASet.SHA);
		ini->WriteString(PLUGINNAME,"SZA",FGTJASet.SZA);
		ini->WriteString(PLUGINNAME,"MAC",FGTJASet.MAC);
		ini->UpdateFile();
	}
	__finally
	{
		delete ini;
	}
}
