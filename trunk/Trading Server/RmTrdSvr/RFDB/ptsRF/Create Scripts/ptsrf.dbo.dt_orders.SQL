/****** 对象:  Table [dbo].[dt_orders]    脚本日期: 11/01/2008 17:19:31 ******/
IF  EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dt_orders]') AND type in (N'U'))
DROP TABLE [dbo].[dt_orders]
GO
/****** 对象:  Table [dbo].[dt_orders]    脚本日期: 11/01/2008 17:19:32 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
SET ANSI_PADDING ON
GO
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dt_orders]') AND type in (N'U'))
BEGIN
CREATE TABLE [dbo].[dt_orders](
	[ID] [bigint] IDENTITY(1,1) NOT NULL,
	[OrgOrderID] [nvarchar](50) NULL,
	[Account] [nvarchar](50) NOT NULL,
	[Instid] [nvarchar](50) NOT NULL,
	[Market] [nvarchar](2) NOT NULL,
	[OrderDate] [int] NOT NULL,
	[OrderTime] [int] NOT NULL,
	[OrderType] [nvarchar](50) NOT NULL,
	[OrderState] [char](1) NOT NULL CONSTRAINT [DF_dtOrders_OrderState]  DEFAULT ((0)),
	[OrderPrice] [numeric](13, 5) NOT NULL,
	[OrderQty] [numeric](18, 0) NOT NULL,
	[ExecAvgPrice] [numeric](13, 5) NOT NULL CONSTRAINT [DF_dt_orders_ExecAvgPrice]  DEFAULT ((0)),
	[ExecQty] [numeric](18, 0) NOT NULL CONSTRAINT [DF_dt_orders_ExecQty]  DEFAULT ((0)),
	[CancleQty] [numeric](18, 0) NOT NULL CONSTRAINT [DF_dt_orders_CancleQty]  DEFAULT ((0)),
 CONSTRAINT [PK_dtOrders] PRIMARY KEY CLUSTERED 
(
	[ID] ASC
)WITH (IGNORE_DUP_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY]
END
GO
SET ANSI_PADDING OFF
GO

/****** 对象:  Index [id_account]    脚本日期: 11/01/2008 17:19:32 ******/
IF NOT EXISTS (SELECT * FROM sys.indexes WHERE object_id = OBJECT_ID(N'[dbo].[dt_orders]') AND name = N'id_account')
CREATE NONCLUSTERED INDEX [id_account] ON [dbo].[dt_orders] 
(
	[OrgOrderID] ASC,
	[Account] ASC,
	[OrderState] ASC,
	[OrderDate] ASC
)WITH (IGNORE_DUP_KEY = OFF) ON [PRIMARY]
GO
EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'mtSHA = ''e'',  //上海证券交易所
mtSZA = ''f'',  //深圳证券交易所
mtJJS = ''j'',  //上海金融交易所
mtFSH = ''g'',  //上海商品期货交易所
mtFZZ = ''i'',  //郑州商品期货交易所
mtFDL = ''h'',  //大连商品期货交易所
mtHK  = ''k'',  //香港联合交易所
	mtNo  = ''N''' ,@level0type=N'SCHEMA', @level0name=N'dbo', @level1type=N'TABLE', @level1name=N'dt_orders', @level2type=N'COLUMN', @level2name=N'Market'

GO
EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'	otBuy      = ''B'', //买
	otSell     = ''S'', //卖
	otETFSub   = ''C'', //ETF申购
	otETFRed   = ''R'', //ETF赎回
	otOpenBuy  = ''1'', //开仓买
	otOpenSell = ''2'', //开仓卖
	otCloseBuy = ''3'', //平仓买
	otCloseSell= ''4'', //平仓卖
	otLoanBuy  = ''5'', //融资买
	otLoanSell = ''6'', //融券卖
	otBuyRepay = ''7'', //买券还券
	otSellRepay= ''8'', //卖券还款
	otNo       = ''N''  //其他' ,@level0type=N'SCHEMA', @level0name=N'dbo', @level1type=N'TABLE', @level1name=N'dt_orders', @level2type=N'COLUMN', @level2name=N'OrderType'

GO
EXEC sys.sp_addextendedproperty @name=N'MS_Description', @value=N'	oswb = ''0'', //未报(等待申报中...)
	osyb = ''1'', //已报(已经报了,但没有任何成交,等待成交中...)
	osdc = ''2'', //待撤(撤单命令已下等待撤单)
	osbw = ''3'', //部撤(确定状态,部分撤销，部分成交)
	osyc = ''4'', //已撤(确定状态,全部撤销了)
	oscj = ''5'', //已成(确定状态,完全成交)
	osyf = ''6'', //已废(确定状态,系统撤单)
  osbc = ''7'', //部成(已经有成交了,正在成交中...
  osno = ''N''  //未知' ,@level0type=N'SCHEMA', @level0name=N'dbo', @level1type=N'TABLE', @level1name=N'dt_orders', @level2type=N'COLUMN', @level2name=N'OrderState'

GO
