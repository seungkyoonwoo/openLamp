DROP PROCEDURE sp_mSync_VerCopy
GO
CREATE PROCEDURE sp_mSync_VerCopy(
		@m_strAppName varchar(30),
		@m_nVersion INT,
		@m_nNewVersion INT
)
as
declare @m_nNewVersionID INT
declare @m_nVersionID INT
declare @nDBID INT, @nNewDBID INT, @nTableID INT, @nNewTableID INT

declare @strDSNName varchar(30), @strDSNUid  varchar(30), @strDSNPwd  varchar(30), @strCliDSN  varchar(30)
declare @strSvrTableName varchar(30), @strCliTableName varchar(30)
declare @strEvent varchar(1), @strScript varchar(4000)

begin
	begin transaction

	-- ���� application�� versionID�� ���Ѵ�
	IF NOT EXISTS 
	(SELECT versionID FROM openMSYNC_version
	WHERE application = @m_strAppName AND version = @m_nVersion)
	BEGIN
		PRINT @m_strAppName+'�� ���� (' + str(@m_nVersion) + ')�� ������ ã�� �� �����ϴ�'
		GOTO sp_END
	END

	IF (@m_nNewVersion <=0 OR @m_nNewVersion >50)
	BEGIN
		PRINT 'Application�� version�� 1�� 50 ������ �����Դϴ�.'
		GOTO sp_END
	END

	if @@ERROR <> 0	 GOTO sp_END

	SELECT @m_nVersionID = versionID FROM openMSYNC_version
	WHERE application = @m_strAppName AND version = @m_nVersion
	-- max version ID�� ���Ѵ� ==> m_nNewVersionID
	SELECT @m_nNewVersionID=max(versionID)+1 FROM openMSYNC_version  
	if @@ERROR <> 0	GOTO sp_END

	-- m_nNewVersionID�� �� version�� insert
	INSERT INTO openMSYNC_version VALUES(@m_nNewVersionID, @m_strAppName, @m_nNewVersion)	

	if @@ERROR <> 0	GOTO sp_END

	-- ���� application�� DB, DSN ������ fetch
	DECLARE DB_cursor CURSOR FOR
	SELECT DBID, svrDSN, DSNUserID, DSNPwd, cliDSN 
	FROM openMSYNC_DSN 
	WHERE VersionID = @m_nVersionID ORDER BY svrDSN

	if @@ERROR <> 0	GOTO sp_END

	OPEN DB_cursor

	if @@ERROR <> 0	GOTO sp_END
	
	FETCH NEXT FROM DB_cursor
	INTO @nDBID, @strDSNName, @strDSNUid, @strDSNPwd, @strCliDSN

	if @@ERROR <> 0	GOTO sp_DB_END

	WHILE @@FETCH_STATUS = 0
	BEGIN
		-- max DBID�� ���Ѵ� ==> nNewDBID
		SELECT @nNewDBID = max(DBID)+1 FROM openMSYNC_dsn 
		if @@ERROR <> 0	GOTO sp_DB_END

		-- nNewDBID�� �� DSN�� insert
		INSERT INTO openMSYNC_DSN VALUES(@nNewDBID, @m_nNewVersionID, @strDSNName, @strDSNUid, @strDSNPwd, @strCliDSN)
		if @@ERROR <> 0	GOTO sp_DB_END

		-- ���� DSN�� tableID, tableName ������ fetch
		DECLARE TABLE_cursor CURSOR FOR
		SELECT TableID, SvrTableName, CliTableName 
		FROM openMSYNC_table 
		WHERE DBID = @nDBID ORDER BY SvrTableName
		if @@ERROR <> 0	GOTO sp_DB_END
  
		OPEN TABLE_cursor

		FETCH NEXT FROM TABLE_cursor
		INTO @nTableID, @strSvrTableName, @strCliTableName
		if @@ERROR <> 0	GOTO sp_DB_TABLE_END

		WHILE @@FETCH_STATUS = 0
		BEGIN
			-- max TableID�� ���Ѵ� ==> nNewTableID
			SELECT @nNewTableID = max(TableID)+1 FROM openMSYNC_table  
			if @@ERROR <> 0	GOTO sp_DB_TABLE_END

			-- nNewTableID�� �� table�� insert
			INSERT INTO openMSYNC_table VALUES(@nNewTableID, @nNewDBID, @strSvrTableName, @strCliTableName)
			if @@ERROR <> 0	GOTO sp_DB_TABLE_END

			-- ���� table�� script ������ fetch
			DECLARE SCRIPT_cursor CURSOR FOR
			SELECT Event, Script 
			FROM openMSYNC_script 
			WHERE TableID = @nTableID
			if @@ERROR <> 0	GOTO sp_DB_TABLE_END

			OPEN SCRIPT_cursor
			if @@ERROR <> 0	GOTO sp_DB_TABLE_END

			FETCH NEXT FROM SCRIPT_cursor
			INTO @strEvent, @strScript
			if @@ERROR <> 0	GOTO sp_DB_TABLE_SCRIPT_END

			WHILE @@FETCH_STATUS = 0
			BEGIN
				-- �� script ������ insert
				INSERT INTO openMSYNC_script VALUES(@nNewTableID, @strEvent, @strScript)
				if @@ERROR <> 0	GOTO sp_DB_TABLE_SCRIPT_END
				FETCH NEXT FROM SCRIPT_cursor
				INTO @strEvent, @strScript				
				if @@ERROR <> 0	GOTO sp_DB_TABLE_SCRIPT_END
			END
			CLOSE SCRIPT_cursor
			DEALLOCATE SCRIPT_cursor

			FETCH NEXT FROM TABLE_cursor
			INTO @nTableID, @strSvrTableName, @strCliTableName
			if @@ERROR <> 0	GOTO sp_DB_TABLE_END
		END
		CLOSE TABLE_cursor
		DEALLOCATE TABLE_cursor

		FETCH NEXT FROM DB_cursor
		INTO @nDBID, @strDSNName, @strDSNUid, @strDSNPwd, @strCliDSN
		if @@ERROR <> 0	GOTO sp_DB_END
	END

	CLOSE DB_cursor
	DEALLOCATE DB_cursor
	commit;
	goto MAIN_END

sp_DB_TABLE_SCRIPT_END:
	PRINT '���� �߿� ������ �߻��Ͽ����ϴ�. : script ���� ����'
	CLOSE SCRIPT_cursor
	DEALLOCATE SCRIPT_cursor
sp_DB_TABLE_END:
	PRINT '���� �߿� ������ �߻��Ͽ����ϴ�. : table ���� ����'
	CLOSE TABLE_cursor
	DEALLOCATE TABLE_cursor
sp_DB_END:
	PRINT '���� �߿� ������ �߻��Ͽ����ϴ�.'
	CLOSE DB_cursor
	DEALLOCATE DB_cursor
sp_END:
	rollback
MAIN_END:
end

