��CREATE TABLE [dbo].[server] (
	[id] [int] NOT NULL ,
	[name] [varchar] (25) NOT NULL ,
	[ip] [varchar] (15) NOT NULL ,
	[inner_ip] [varchar] (15) NOT NULL ,
	[ageLimit] [tinyint] NOT NULL ,
	[pk_flag] [tinyint] NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[ssn] (
	[ssn] [char] (13) NOT NULL ,
	[name] [varchar] (15) NOT NULL ,
	[email] [varchar] (50) NOT NULL ,
	[newsletter] [tinyint] NOT NULL ,
	[job] [int] NOT NULL ,
	[phone] [varchar] (16) NOT NULL ,
	[mobile] [varchar] (20) NULL ,
	[reg_date] [datetime] NOT NULL ,
	[zip] [varchar] (6) NOT NULL ,
	[addr_main] [varchar] (255) NOT NULL ,
	[addr_etc] [varchar] (255) NOT NULL ,
	[account_num] [int] NOT NULL ,
	[status_flag] [int] NOT NULL ,
	[final_news_date] [datetime] NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[user_account] (
	[uid] [int] IDENTITY (1, 1) NOT NULL ,
	[account] [varchar] (14) NOT NULL ,
	[pay_stat] [int] NOT NULL ,
	[login_flag] [int] NOT NULL ,
	[warn_flag] [int] NOT NULL ,
	[block_flag] [int] NOT NULL ,
	[block_flag2] [int] NOT NULL ,
	[last_login] [datetime] NULL ,
	[last_logout] [datetime] NULL ,
	[subscription_flag] [int] NOT NULL ,
	[last_world] [tinyint] NULL ,
	[last_game] [int] NULL ,
	[last_ip] [varchar] (15) NULL ,
	[block_end_date] [datetime] NULL ,
	[queue_level] [int] NOT NULL
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[user_auth] (
	[account] [varchar] (14) NOT NULL ,
	[password] [binary] (128) NOT NULL ,
	[hash_type] [tinyint] NOT NULL DEFAULT 0,
	[salt] [int] NOT NULL DEFAULT 0,
	[quiz1] [varchar] (255) NOT NULL ,
	[quiz2] [varchar] (255) NOT NULL ,
	[answer1] [binary] (128) NOT NULL ,
	[answer2] [binary] (128) NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[user_count] (
	[record_time] [datetime] NOT NULL ,
	[server_id] [tinyint] NOT NULL ,
	[world_user] [int] NOT NULL ,
	[limit_user] [int] NOT NULL ,
	[auth_user] [int] NOT NULL ,
	[wait_user] [int] NOT NULL ,
	[dayofweek] [tinyint] NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[user_data] (
	[uid] [int] NOT NULL ,
	[user_data] [binary] (16) NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[user_info] (
	[account] [varchar] (14) NOT NULL ,
	[create_date] [datetime] NOT NULL ,
	[ssn] [varchar] (13) NOT NULL ,
	[status_flag] [tinyint] NOT NULL ,
	[kind] [int] NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[user_time] (
	[uid] [int] NOT NULL ,
	[account] [varchar] (14) NOT NULL ,
	[present_time] [int] NOT NULL ,
	[next_time] [int] NULL ,
	[total_time] [int] NOT NULL ,
	[op_date] [datetime] NOT NULL ,
	[flag] [tinyint] NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[worldstatus] (
	[idx] [int] NOT NULL ,
	[server] [varchar] (50) NOT NULL ,
	[status] [tinyint] NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [dbo].[ssn] WITH NOCHECK ADD 
	CONSTRAINT [PK_ssn] PRIMARY KEY  CLUSTERED 
	(
		[ssn]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[user_account] WITH NOCHECK ADD 
	CONSTRAINT [PK_user_account_] PRIMARY KEY  CLUSTERED 
	(
		[account]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[user_auth] WITH NOCHECK ADD 
	CONSTRAINT [PK_user_auth] PRIMARY KEY  CLUSTERED 
	(
		[account]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[user_info] WITH NOCHECK ADD 
	CONSTRAINT [PK_user_info] PRIMARY KEY  CLUSTERED 
	(
		[account]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[worldstatus] WITH NOCHECK ADD 
	CONSTRAINT [PK_worldstatus] PRIMARY KEY  CLUSTERED 
	(
		[idx]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[server] WITH NOCHECK ADD 
	CONSTRAINT [DF_server_ageLimit] DEFAULT (0) FOR [ageLimit],
	CONSTRAINT [DF_server_pk_flag] DEFAULT (1) FOR [pk_flag]
GO

ALTER TABLE [dbo].[ssn] WITH NOCHECK ADD 
	CONSTRAINT [DF_ssn_newsletter] DEFAULT (0) FOR [newsletter],
	CONSTRAINT [DF_ssn_reg_date] DEFAULT (getdate()) FOR [reg_date],
	CONSTRAINT [DF_ssn_account_num] DEFAULT (0) FOR [account_num],
	CONSTRAINT [DF_ssn_status_flag] DEFAULT (0) FOR [status_flag]
GO

ALTER TABLE [dbo].[user_account] WITH NOCHECK ADD 
	CONSTRAINT [DF_user_account__pay_stat] DEFAULT (0) FOR [pay_stat],
	CONSTRAINT [DF_user_account__login_flag] DEFAULT (0) FOR [login_flag],
	CONSTRAINT [DF_user_account__warn_flag] DEFAULT (0) FOR [warn_flag],
	CONSTRAINT [DF_user_account__block_flag] DEFAULT (0) FOR [block_flag],
	CONSTRAINT [DF_user_account__block_flag2] DEFAULT (0) FOR [block_flag2],
	CONSTRAINT [DF_user_account_subscription_flag] DEFAULT (0) FOR [subscription_flag],
	CONSTRAINT [DF_user_account_queue_level] DEFAULT (1) FOR [queue_level]
GO

ALTER TABLE [dbo].[user_count] WITH NOCHECK ADD 
	CONSTRAINT [DF_user_count_record_time] DEFAULT (getdate()) FOR [record_time],
	CONSTRAINT [DF_user_count_dayofweek] DEFAULT (datepart(weekday,getdate())) FOR [dayofweek]
GO

ALTER TABLE [dbo].[user_info] WITH NOCHECK ADD 
	CONSTRAINT [DF_user_info_create_date] DEFAULT (getdate()) FOR [create_date],
	CONSTRAINT [DF_user_info_status_flag] DEFAULT (0) FOR [status_flag],
	CONSTRAINT [DF_user_info_kind] DEFAULT (0) FOR [kind]
GO

ALTER TABLE [dbo].[user_time] WITH NOCHECK ADD 
	CONSTRAINT [PK_user_time] PRIMARY KEY  NONCLUSTERED 
	(
		[uid]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[worldstatus] WITH NOCHECK ADD 
	CONSTRAINT [DF_worldstatus_status] DEFAULT (0) FOR [status]
GO

 CREATE  INDEX [idx_uid] ON [dbo].[user_account]([uid]) ON [PRIMARY]
GO

 CREATE  INDEX [idx_record] ON [dbo].[user_count]([record_time]) ON [PRIMARY]
GO

 CREATE  INDEX [idx_serverid] ON [dbo].[user_count]([server_id]) ON [PRIMARY]
GO

 CREATE  INDEX [idx_dayofweek] ON [dbo].[user_count]([dayofweek]) ON [PRIMARY]
GO

 CREATE  UNIQUE  INDEX [idx_account] ON [dbo].[user_time]([account]) ON [PRIMARY]
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

-- Created By darkangel
-- 2003.06.19
-- Get Account's Password

CREATE PROCEDURE [dbo].[ap_GPwd]  @account varchar(14), @pwd binary(128) output, @hash_type tinyint output, @salt int output
AS
SELECT @pwd=password, @hash_type=hash_type, @salt=salt FROM user_auth with (nolock) WHERE account=@account
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE [dbo].[ap_GUserTime]  @uid int, @userTime int OUTPUT
AS
SELECT @userTime=total_time FROM user_time WITH (nolock) 
WHERE uid = @uid
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE [dbo].[ap_GStat]
@account varchar(14), 
@uid int OUTPUT, 
@payStat int OUTPUT, 
@loginFlag int OUTPUT, 
@warnFlag int OUTPUT, 
@blockFlag int OUTPUT, 
@blockFlag2 int OUTPUT, 
@subFlag int OUTPUT, 
@lastworld tinyint OUTPUT,
@block_end_date datetime OUTPUT,
@queue_level int OUTPUT
 AS
SELECT @uid=uid, 
	@payStat=pay_stat,
	@loginFlag = login_flag, 
	@warnFlag = warn_flag, 
	@blockFlag = block_flag, 
	@blockFlag2 = block_flag2, 
	@subFlag = subscription_flag , 
	@lastworld=last_world, 
	@block_end_date=block_end_date,
	@queue_level=queue_level
	FROM user_account WITH (nolock)
WHERE account=@account
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE [dbo].[ap_SLog] 
@uid int, @lastlogin datetime, @lastlogout datetime, @LastGame int, @LastWorld tinyint, @LastIP varchar(15)
AS
UPDATE user_account 
SET last_login = @lastlogin, last_logout=@lastlogout, last_world=@lastWorld, last_game=@lastGame, last_ip=@lastIP
WHERE uid=@uid
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE [dbo].[ap_SUserTime] @useTime int, @uid int
AS
UPDATE user_time SET total_time = total_time - @useTime, present_time = present_time - @useTime
WHERE uid = @uid
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

 /* 2003-08-20 JuYoung */
CREATE TRIGGER [dt_UpdatePayStatus] ON [dbo].[user_time] 
FOR  UPDATE
AS
declare @account varchar(14), @present_time integer, @total_time integer, @old_pay_id char(16), @pay_id char(16), @method integer, @feature integer, @duration integer
declare @flag tinyint
set nocount on

select @account=account, @present_time=present_time, @total_time=total_time from inserted
if @@rowcount=0 or (@present_time > 0)
	goto exit_trigger

select @account=account from user_account with (nolock) where account=@account and block_flag=0 and block_flag2=0 
if @@rowcount = 0
	goto exit_trigger

select @old_pay_id=pay_id from user_pay where account=@account

while @present_time <= 0 
begin
	select top 1 @pay_id=pay_id, @method=method, @duration=duration, @flag=flag  from 
	(	select  pay_id, method, duration, 1 as flag from user_pay_reserve  where account=@account
		union
		select pay_id, 9100 as method, duration, 0 as flag from user_pay_recomp  where account=@account
	) a
	order by method desc

	if @@rowcount > 0
	begin
		if (@method/100)%10= 1
		begin
			delete user_pay where account=@account
			delete user_time where account=@account and flag=1
			update pay_master set 	play_end=dateadd(ss, @present_time, getdate()) ,real_end_date=dateadd(ss, @present_time, getdate()), loc=3 where pay_id=@old_pay_id 
			update pay_master set  play_start=dateadd(ss, @present_time, getdate()), play_end=dateadd(day, @duration-1, getdate()), real_end_date=dateadd(day, @duration-1, getdate()), loc=1 where pay_id=@pay_id
			insert into user_pay (pay_id, account, start_date, method, duration) values (@pay_id, @account, getdate(), @method, @duration)

			if @flag = 1
				delete user_pay_reserve where pay_id=@pay_id and account=@account
			else if @flag = 0
				delete user_pay_recomp where pay_id=@pay_id and account=@account						

			update user_account set pay_stat=@method where account=@account
			break
		end
		else if (@method/100)%10= 2
		begin
			delete user_pay where account=@account
			update pay_master set play_end=dateadd(ss, @present_time, getdate()), real_end_date=dateadd(ss, @present_time, getdate()), loc=3 where pay_id=@old_pay_id
			update pay_master set   play_start=dateadd(ss, @present_time, getdate()), loc=1 where pay_id=@pay_id
			insert into user_pay (pay_id, account, start_date, method, duration) values (@pay_id, @account,dateadd(ss, @present_time, getdate()), @method, @duration)

			if @flag = 1
				delete user_pay_reserve where pay_id=@pay_id and account=@account
			else if @flag = 0
				delete user_pay_recomp where pay_id=@pay_id and account=@account						

			set @old_pay_id = @pay_id
			set @present_time = @present_time+@duration
			update user_time set present_time=@present_time  where account=@account and flag=1
			update user_account set pay_stat=@method where account=@account
		end
		else
			goto exit_trigger
		
	end
	else
	begin
		delete user_time where account=@account and flag=1
		delete user_pay where account=@account
		update pay_master set play_end=getdate(), real_end_date=getdate(), loc=3 where pay_id=@old_pay_id
		update user_account set pay_stat=0 where account=@account
		break
	end
end
exit_trigger:
	return







GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO
