/****** 对象:  StoredProcedure [dbo].[dp_AddExecReport]    脚本日期: 11/27/2008 16:41:11 ******/
IF  EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dp_AddExecReport]') AND type in (N'P', N'PC'))
DROP PROCEDURE [dbo].[dp_AddExecReport]
GO
/****** 对象:  StoredProcedure [dbo].[GetExecReportByOrdid]    脚本日期: 11/27/2008 16:41:11 ******/
IF  EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[GetExecReportByOrdid]') AND type in (N'P', N'PC'))
DROP PROCEDURE [dbo].[GetExecReportByOrdid]
GO
/****** 对象:  StoredProcedure [dbo].[dp_AddOrders]    脚本日期: 11/27/2008 16:41:12 ******/
IF  EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dp_AddOrders]') AND type in (N'P', N'PC'))
DROP PROCEDURE [dbo].[dp_AddOrders]
GO
/****** 对象:  StoredProcedure [dbo].[dp_CancelOrder]    脚本日期: 11/27/2008 16:41:12 ******/
IF  EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dp_CancelOrder]') AND type in (N'P', N'PC'))
DROP PROCEDURE [dbo].[dp_CancelOrder]
GO
/****** 对象:  StoredProcedure [dbo].[dp_GetExecReport]    脚本日期: 11/27/2008 16:41:12 ******/
IF  EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dp_GetExecReport]') AND type in (N'P', N'PC'))
DROP PROCEDURE [dbo].[dp_GetExecReport]
GO
/****** 对象:  StoredProcedure [dbo].[dp_GetOrders]    脚本日期: 11/27/2008 16:41:12 ******/
IF  EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dp_GetOrders]') AND type in (N'P', N'PC'))
DROP PROCEDURE [dbo].[dp_GetOrders]
GO
/****** 对象:  StoredProcedure [dbo].[dp_GetUnfilledOrder]    脚本日期: 11/27/2008 16:41:13 ******/
IF  EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dp_GetUnfilledOrder]') AND type in (N'P', N'PC'))
DROP PROCEDURE [dbo].[dp_GetUnfilledOrder]
GO
/****** 对象:  StoredProcedure [dbo].[dp_AddExecReport]    脚本日期: 11/27/2008 16:41:13 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dp_AddExecReport]') AND type in (N'P', N'PC'))
BEGIN
EXEC dbo.sp_executesql @statement = N'CREATE PROCEDURE [dbo].[dp_AddExecReport] 
	(
	@ordid bigint,
	@instid nvarchar(50),
	@execid nvarchar(50),
	@execqty numeric(18,0),
	@execprice numeric(13,5)
	)
AS
	-- update dt_orders
	IF @execqty < 0 
	BEGIN
		UPDATE dt_orders 
			   SET CancleQty = abs(@execqty) , 
			   OrderState= CASE WHEN (ExecQty > 0) AND ((abs(@execqty) + ExecQty) = OrderQty) THEN ''3'' 
							 WHEN (ExecQty = 0) AND (ExecQty = OrderQty) THEN ''4''
							 ELSE ''N'' 
						   END
			   WHERE (id= @ordid)	
	END ELSE
	BEGIN
		UPDATE dt_orders SET OrderPrice =  (OrderPrice * OrderQty + @execprice * @execqty ) / (OrderQty + @execqty ),
			   ExecQty = ExecQty + @execqty,
			   OrderState = CASE WHEN (ExecQty + @execqty + abs(CancleQty )) < OrderQty THEN ''7''
								 WHEN (abs(CancleQty)>0) AND ((ExecQty + @execqty + abs(CancleQty )) = OrderQty) THEN ''3''
								 WHEN ((ExecQty + @execqty ) = OrderQty) THEN ''5''
								 ELSE ''N''
						    END
		WHERE (id = @ordid)	
	END	
	
	declare @acc nvarchar(50)
	declare @exectype nvarchar(2)
	SELECT @acc = Account, @exectype= OrderType FROM dt_orders WHERE id = @ordid
	
	INSERT INTO dt_ordreport (OrderID,Instid,Execid,ExecDate,ExecTime,ExecType,ExecQty,ExecPrice,Account) 
	VALUES 
		( @ordid, @instid,@execid,
		CONVERT(int,CONVERT(char(8),GetDate(),112)),
		CONVERT(int,REPLACE(CONVERT(char(8),GetDate(),108),'':'','''')),@exectype,@execqty,@execprice,
		@acc)
	
	RETURN 0
' 
END
GO
/****** 对象:  StoredProcedure [dbo].[GetExecReportByOrdid]    脚本日期: 11/27/2008 16:41:14 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[GetExecReportByOrdid]') AND type in (N'P', N'PC'))
BEGIN
EXEC dbo.sp_executesql @statement = N'CREATE PROCEDURE dbo.GetExecReportByOrdid 
	(
	@acc nvarchar(50),
	@ordid bigint,
	@date int=0
	)
AS
	IF @date <=0 
		SET @date = CONVERT(int,CONVERT(char(8),GetDate(),112))
		
	SELECT   ID, OrderID, Instid, Execid, ExecTime, ExecDate, ExecType, ExecQty, ExecPrice ,Account
	FROM         dt_ordreport 
	WHERE (OrderID = @ordid) AND (Account =@acc ) AND (ExecDate = @date) 
	 
	RETURN @@RowCount
' 
END
GO
/****** 对象:  StoredProcedure [dbo].[dp_AddOrders]    脚本日期: 11/27/2008 16:41:15 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dp_AddOrders]') AND type in (N'P', N'PC'))
BEGIN
EXEC dbo.sp_executesql @statement = N'CREATE PROCEDURE dbo.dp_AddOrders 
(
	@acc nvarchar(50),
	@instid nvarchar(50),
	@ordprice numeric(13,5),
	@ordqty numeric(18,0),
	@orgordid nvarchar(20),
	@ordtype nvarchar(2),
	@market nvarchar(2),
	@ordstate char(1),
	@id bigint OUTPUT
)
AS
	INSERT INTO dt_orders ( OrgOrderID,Account,Instid, Market,OrderDate,OrderTime,OrderType,
							OrderState,OrderPrice,OrderQty )
	VALUES( @orgordid,@acc,@instid,@market,
			CONVERT(int,CONVERT(char(8),GetDate(),112)),
	        CONVERT(int,REPLACE(CONVERT(char(8),GetDate(),108),'':'','''')),
	        @ordtype,@ordstate,@ordprice,@ordqty)
	SET @id = @@IDENTITY
	RETURN 0
' 
END
GO
/****** 对象:  StoredProcedure [dbo].[dp_CancelOrder]    脚本日期: 11/27/2008 16:41:17 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dp_CancelOrder]') AND type in (N'P', N'PC'))
BEGIN
EXEC dbo.sp_executesql @statement = N'-- =============================================
-- Author:		<Author,,Name>
-- Create date: <Create Date,,>
-- Description:	<Description,,>
-- =============================================
CREATE PROCEDURE dp_CancelOrder
(
	@ordid bigint
)	
AS
BEGIN
	SET NOCOUNT ON;
	DECLARE @ret int
	DECLARE @ordstate char(1)
	
	UPDATE dt_orders SET OrderState = ''2'' 
	WHERE (ID = @ordid) AND ( OrderState NOT IN (''3'',''4'',''5'',''6'',''N'',''2''));

	IF @@RowCount = 0
		return -1
	
	return 0
END
' 
END
GO
/****** 对象:  StoredProcedure [dbo].[dp_GetExecReport]    脚本日期: 11/27/2008 16:41:17 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dp_GetExecReport]') AND type in (N'P', N'PC'))
BEGIN
EXEC dbo.sp_executesql @statement = N'CREATE PROCEDURE [dbo].[dp_GetExecReport]
	(
	@acc nvarchar(50),
	@startindex bigint,
	@date int=0,
	@maxcount int
	)
AS
	IF @date <=0 
		SET @date = CONVERT(int,CONVERT(char(8),GetDate(),112))
		
	SELECT   TOP(@maxcount)  ID, OrderID, Instid, Execid, ExecTime, ExecDate, ExecType, ExecQty, ExecPrice ,Account
	FROM         dt_ordreport 
	WHERE (ID > @startindex) AND (Account =@acc ) AND (ExecDate = @date) ORDER BY ID ASC
	 
	RETURN @@RowCount
 ' 
END
GO
/****** 对象:  StoredProcedure [dbo].[dp_GetOrders]    脚本日期: 11/27/2008 16:41:19 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dp_GetOrders]') AND type in (N'P', N'PC'))
BEGIN
EXEC dbo.sp_executesql @statement = N'CREATE PROCEDURE dbo.dp_GetOrders
	(
	@acc nvarchar(50),
	@startindex bigint,
	@date int=0,
	@maxcount int
	)
AS
	IF @date <=0 
		SET @date = CONVERT(int,CONVERT(char(8),GetDate(),112))
		
	IF @maxcount = 1 
		SELECT  ID, OrgOrderID, Account, Instid, Market, OrderDate, OrderTime, OrderType, OrderState, OrderPrice, OrderQty, ExecAvgPrice, ExecQty, CancleQty
		FROM         dt_orders
		WHERE     (ID = @startindex) AND (OrderDate = @date ) AND (Account = @acc)	
	ELSE
		SELECT  TOP(@maxcount) ID, OrgOrderID, Account, Instid, Market, OrderDate, OrderTime, OrderType, OrderState, OrderPrice, OrderQty, ExecAvgPrice, ExecQty, CancleQty
		FROM         dt_orders
		WHERE     (ID > @startindex) AND (OrderDate = @date ) AND (Account = @acc) ORDER BY ID ASC
	
	RETURN @@RowCount
' 
END
GO
/****** 对象:  StoredProcedure [dbo].[dp_GetUnfilledOrder]    脚本日期: 11/27/2008 16:41:19 ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[dp_GetUnfilledOrder]') AND type in (N'P', N'PC'))
BEGIN
EXEC dbo.sp_executesql @statement = N'CREATE PROCEDURE dbo.dp_GetUnfilledOrder
	(
		@acc nvarchar(50),
		@date int = 0
	)
AS
	IF (@date <=0)
	BEGIN
		SELECT  ID, OrgOrderID, Account, Instid, Market, OrderDate, OrderTime, 
			OrderType, OrderState, OrderPrice, OrderQty, ExecAvgPrice, ExecQty, CancleQty
		FROM dt_orders 
		WHERE (OrderState IN (''3'',''4'',''5'',''6'')) AND (Account = @acc) 
	END ELSE
	BEGIN
		SELECT  ID, OrgOrderID, Account, Instid, Market, OrderDate, OrderTime, 
			OrderType, OrderState, OrderPrice, OrderQty, ExecAvgPrice, ExecQty, CancleQty
		FROM dt_orders 
		WHERE (OrderState IN (''3'',''4'',''5'',''6'')) AND (OrderDate = @date) AND (Account = @acc) 
	END
	RETURN @@ROWCOUNT
' 
END
GO
