/*
    Easy Database - SQLITE manager (easydb.inc)
    * SQLITE has been the fastest and very powerful database language for samp!
	* This include allows you to write and run necessary queries without any SQL language
	* This is fast and has wide range of options and functions.
	* Also, this supports multiple SQL tables handling at one time.

	Author: (creator)
 	* Gammix

	Contributors:
 	* Slice - Took help from BUD include (a good user database include)

	(c) Copyright 2015
 	* This file is provided as is (no warranties).
*/
/*
FUNCTIONS:
native DB::Open(name[]);
native DB::Close();
native DB::VerifyTable(const table[], const key[]);
native DB::VerifyColumn(table, const column[], DB::e_COLUMN_TYPES:type, { _,Float }:...);
native DB::RetrieveKey(table, const column[], { _,Float }:...);
native DB::CreateRow(table, const column[], { _,Float }:...);
native DB::DeleteRow(table, key);
native DB::GetIntEntry(table, key, const column[]);
native Float:DB::GetFloatEntry(table, key, const column[]);
native DB::GetStringEntry(table, key, const column[], str[], len = sizeof(str));
native DB::SetIntEntry(table, key, const column[], value);
native DB::SetFloatEntry(table, key, const column[], Float:value);
native DB::SetStringEntry(table, key, const column[], value[]);
native DBResult:DB::Query(query[], bool:keepresult = false);
native DB::CountTables();
native DB::CountColumns(table);
native DB::CountRows(table);
native _
native DB::Hash(buffer[], len = sizeof(buffer), const str[]);
native DB::Escape(str[], ichar = '\'', len = sizeof(str));
*/

//naming ease
#define DB::      					DB_
#define dbstatic    				stock static
#define dbglobal     				stock

//notification defines
#define DB_Notice(%0)       		(printf("[easydb.inc] - NOTICE: " %0))
#define DB_Warning(%0)      		(printf("[easydb.inc] - WARNING: " %0))
#define DB_Error(%0)        		(printf("[easydb.inc] - ERROR: " %0))

//constants (you an define them before inclution also)
#if !defined DB_MAX_DATABASE_NAME
	#define DB_MAX_DATABASE_NAME 	(16)
#endif

#if !defined DB_MAX_COLUMNS
	#define DB_MAX_COLUMNS  		(25)
#endif

#if !defined DB_MAX_COLUMN_NAME
	#define DB_MAX_COLUMN_NAME  	(16)
#endif

#if !defined DB_MAX_ENTRY_STRING
	#define DB_MAX_ENTRY_STRING  	(64)
#endif

#if !defined DB_MAX_TABLES
	#define DB_MAX_TABLES  			(10)
#endif

#if !defined DB_MAX_TABLE_NAME
	#define DB_MAX_TABLE_NAME  		(16)
#endif

#if !defined DB_BUFFER_SIZE
	#define DB_BUFFER_SIZE  		(3072)
#endif

#if !defined DB_ALIVE_TIME
	#define DB_ALIVE_TIME  			(2000)
#endif

#if !defined DB_TIMEOUT_CHECK
	#define DB_TIMEOUT_CHECK  		(3000)
#endif

#if !defined DB_SYNCHRONOUS
	#define DB_SYNCHRONOUS  		(true)
#endif

//the defualt DB name (no need to change, you can do it in DB::Open)
//the deafult name is there in case the name isn't appropriate or invalid when inputed in DB::Open
#define DEFAULT_DATABASE_NAME  		"server.db"

//invalid defines
#define INVALID_DB           		(DB:0)
#define DB_INVALID_KEY      		(-1)
#define DB_INVALID_TABLE    		(-1)

//column types enum
enum DB::e_COLUMN_TYPES
{
	DB::TYPE_NUMBER,
	DB::TYPE_FLOAT,
	DB::TYPE_STRING
};

//
enum DB::e_TABLE_DATA
{
	DB::TABLE_NAME[DB::MAX_TABLE_NAME],
	DB::TABLE_KEY[DB::MAX_COLUMN_NAME]
};

//declaring vars and arrays
dbstatic
	DB:		g_Database = INVALID_DB,
	      	g_DatabaseTimer = -1,
	      	g_DatabaseName[DB::MAX_DATABASE_NAME] = "bud.db",
 	bool:	g_Initialized = false,

	      	g_ColumnNames[DB::MAX_TABLES][DB::MAX_COLUMNS][DB::MAX_COLUMN_NAME],
	      	g_ColumnCount[DB::MAX_TABLES],

	      	g_TableData[DB::MAX_TABLES][DB::e_TABLE_DATA],
	      	g_TableCount,

	      	g_Query[DB::BUFFER_SIZE]
;

//

//Internal functions
dbstatic bool:DB::Get()
{
	if (g_DatabaseTimer == -1)
	{
		new
			i_Tick = GetTickCount()
		;

		do
		{
			g_Database = db_open(g_DatabaseName);
		}
		while
			(g_Database == INVALID_DB &&
			(GetTickCount() - i_Tick) < DB::TIMEOUT_CHECK)
		;

		if (g_Database == INVALID_DB)
		{
			DB::Error("Unable to open the database \"%s\".", g_DatabaseName);

			return false;
		}
		else
		{
			#if (DB_SYNCHRONOUS)
			{
				db_query(g_Database, "PRAGMA synchronous = 0");
			}
			#else
			{
				db_query(g_Database, "PRAGMA synchronous = 3");
			}
			#endif
		}
	}
	else
	{
		KillTimer(g_DatabaseTimer);
	}

	g_DatabaseTimer = SetTimer("DB_CloseDatabase", DB::ALIVE_TIME, false);

	return true;
}

forward DB::CloseDatabase();
public  DB::CloseDatabase()
{
	g_DatabaseTimer = -1;

	db_close(g_Database);

	g_Database = INVALID_DB;
}

dbstatic bool:DB::IntegrityCheck()
{
	if (! DB::Get())
	{
		return false;
	}

	new
		DBResult:i_DBResult
	;

	i_DBResult = db_query(g_Database, "PRAGMA integrity_check");

	if (i_DBResult)
	{
		new
			i_Row,
			i_Field[64]
		;

		do
		{
			db_get_field(i_DBResult, 0, i_Field, sizeof(i_Field) - 1);

			if (i_Row == 0 &&
				! strcmp("ok", i_Field))
			{
				break;
			}
			else
			{
				DB::Warning("Database integrity check says: %s", i_Field);
			}

			i_Row++;
		}

		while
			(db_next_row(i_DBResult))
		;

		db_free_result(i_DBResult);
		return true;
	}
	return false;
}

dbstatic bool:DB::ReloadTableInfo(table)
{
	if (! g_Initialized)
	{
	    return false;
 	}

	if (! DB::Get())
	{
		return false;
	}

	new
		DBResult:i_DBResult
	;

	format(g_Query, DB::BUFFER_SIZE, "PRAGMA table_info('%s')", g_TableData[table][DB::TABLE_NAME]);
	i_DBResult = db_query(g_Database, g_Query);

	if (i_DBResult)
	{
	    g_ColumnCount[table] = 0;

		new
			i_ColoumnStr[DB::MAX_COLUMN_NAME]
		;

		do
		{
			if (g_ColumnCount[table] + 1 >= DB::MAX_COLUMNS)
			{
				DB::Warning("There are more columns in the \"%s\" table than \"DB::MAX_COLUMNS\". Increase the limit!", g_TableData[table][DB::TABLE_NAME]);

				break;
			}

			db_get_field(i_DBResult, 1, i_ColoumnStr, DB::MAX_COLUMN_NAME - 1);
			format(g_ColumnNames[table][g_ColumnCount[table]], DB::MAX_COLUMN_NAME, i_ColoumnStr);

			g_ColumnCount[table] += 1;
		}
		while
			(db_next_row(i_DBResult))
		;

		db_free_result(i_DBResult);
	 	return true;
	}
	else
	{
		DB::Warning("Failed to get the table info from \"%s\"; some columns could be missing.", g_DatabaseName);
	 	return false;
	}
}

dbstatic DB::SetColumnType(table, const column[], type)
{
	new
	    i_String[40]
	;
	format(i_String, sizeof(i_String), "%i_%s", table, column);

	setproperty(.name = i_String, .value = type);

	return true;
}

dbstatic DB::GetColumnType(table, const column[])
{
	new
	    i_String[40]
	;
	format(i_String, sizeof(i_String), "%i_%s", table, column);

	return getproperty(.name = i_String);
}

//Main include functions
dbglobal bool:DB::Open(name[])
{
	if (g_Initialized)
	{
		DB::Notice("Warning, Initialization aborted; Database is already initialized.");

		return true;
	}

	if (! name[0])
	{
		DB::Warning("The database name was not specified; default will be applied (\""DEFAULT_DATABASE_NAME"\").");

		g_DatabaseName = DEFAULT_DATABASE_NAME;
	}
	else
	{
	    format(g_DatabaseName, sizeof(g_DatabaseName), name);
	}

	if (! fexist(name))
	{
		DB::Notice("The database \"%s\" doesn't exist; it will be created.", g_DatabaseName);

		if (! DB::Get())
		{
			DB::Error("The database \"%s\" wasn't created, the include have unloaded.", g_DatabaseName);
			return false;
		}
	}

	DB::IntegrityCheck();

	g_Initialized = true;
	for (new t; t < DB::MAX_TABLES; t++)
	{
	    g_ColumnCount[t] = 0;
     	g_TableData[t][DB::TABLE_NAME][0] = EOS;
     	g_TableData[t][DB::TABLE_KEY][0] = EOS;

	    for (new c; c < DB::MAX_COLUMNS; c++)
		{
			g_ColumnNames[t][c][0] = EOS;
		}
	}
	g_TableCount = 0;

	print("[easydb.inc] Include loaded (build 1)");

	return true;
}

dbglobal bool:DB::Close()
{
	if (! g_Initialized)
	{
	    return false;
 	}

	if (g_DatabaseTimer != -1)
	{
		KillTimer(g_DatabaseTimer);

		DB::CloseDatabase();
	}

	g_Initialized = false;
	return true;
}

dbglobal DB::VerifyTable(const table[], const key[])
{
	if (! table[0])
	{
		return DB::INVALID_TABLE;
 	}

	if (strlen(table) >= DB::MAX_COLUMN_NAME)
	{
		return DB::INVALID_TABLE;
 	}

	if (! key[0])
	{
		return DB::INVALID_TABLE;
 	}

	if (strlen(key) >= DB::MAX_COLUMN_NAME)
	{
		return DB::INVALID_TABLE;
 	}

	for (new i = 0; i < g_TableCount; i++)
	{
		if (! strcmp(g_TableData[i][DB::TABLE_NAME], table))
		{
			return i;
		}
	}

    if (! DB::Get())
	{
		return DB::INVALID_TABLE;
	}

	format(g_Query, DB::BUFFER_SIZE, "SELECT * FROM `%s` LIMIT 1", table);

	new
	    DBResult:i_DBResult = db_query(g_Database, g_Query)
	;
	if(! i_DBResult)
	{
	    db_free_result(i_DBResult);

	    DB::Notice("The table \"%s\" doesn't exist; attempting to create it.", table);

		format(g_Query, DB::BUFFER_SIZE, "CREATE TABLE `%s` (`%s` INTEGER PRIMARY KEY)", table, key);

		db_free_result(db_query(g_Database, g_Query));
	}

	format(g_TableData[g_TableCount][DB::TABLE_NAME], DB::MAX_TABLE_NAME, table);
	format(g_TableData[g_TableCount][DB::TABLE_KEY], DB::MAX_TABLE_NAME, key);

	DB::ReloadTableInfo(g_TableCount);

	g_TableCount++;

	return (g_TableCount - 1);
}

dbglobal bool:DB::VerifyColumn(table, const column[], DB::e_COLUMN_TYPES:type, { _,Float }:...)
{
 	if (! g_TableData[table][DB::TABLE_NAME][0])
 	{
 	    return false;
 	}

 	if (strlen(g_TableData[table][DB::TABLE_NAME]) >= DB::MAX_TABLE_NAME)
 	{
 	    return false;
 	}

	if (! column[0])
	{
		return false;
 	}

	if (strlen(column) >= DB::MAX_COLUMN_NAME)
	{
		return false;
 	}

	if (! (DB::e_COLUMN_TYPES:0 <= type <= DB::e_COLUMN_TYPES))
	{
		DB::Error("Invalid type given to DB::VerifyColumn, the coloumn couldn't be verified or created.");

		return false;
	}

	if (! g_ColumnCount[table])
	{
		DB::Notice("Because the table info wasn't retrieved, columns cannot be verified.");

		return false;
	}

	new
		bool:i_ColumnCreated = false
	;

recheck:

	for (new i = 0; i < g_ColumnCount[table]; i++)
	{
		if (! strcmp(g_ColumnNames[table][i], column))
		{
			if(DB::GetColumnType(table, column) != _:type)
			{
            	DB::SetColumnType(table, column, _:type);
			}
			return true;
		}
	}

	if (! i_ColumnCreated)
	{
		DB::Notice("The column \"%s\" doesn't exist; attempting to create it.", column);

		if (! DB::Get())
		{
			return false;
		}

		new
		    i_Type
		;

		switch (type)
		{
			case DB::TYPE_NUMBER:
			{
				new
					i_DefaultValue = 0
				;

				if (numargs() == 4)
				{
					i_DefaultValue = getarg(3);
				}

			    i_Type = _:DB::TYPE_NUMBER;

				format(g_Query, DB::BUFFER_SIZE, "ALTER TABLE `%s` ADD COLUMN `%s` INTEGER DEFAULT(%d)", g_TableData[table][DB::TABLE_NAME], column, i_DefaultValue);
			}

			case DB::TYPE_FLOAT:
			{
				new
					Float:i_DefaultValue = 0.0
				;

				if (numargs() != 4)
				{
					i_DefaultValue = Float:getarg(3);
				}

			    i_Type = _:DB::TYPE_FLOAT;

				format(g_Query, DB::BUFFER_SIZE, "ALTER TABLE `%s` ADD COLUMN `%s` REAL DEFAULT(%f)", g_TableData[table][DB::TABLE_NAME], column, i_DefaultValue);
			}

			case DB::TYPE_STRING:
			{
				new
					i_DefaultValue[DB::MAX_ENTRY_STRING * 2] = ""
				;

				if (numargs() == 4)
				{
				    new
						i_Loop = (0x-1)
					;
					while (++i_Loop != 0x100)
					{
					    if(i_Loop < DB::MAX_ENTRY_STRING)
					    {
							i_DefaultValue[i_Loop] = getarg(3, i_Loop);
						}
					}

					i_DefaultValue[DB::MAX_ENTRY_STRING - 1] = EOS;

					DB::Escape(i_DefaultValue);
				}

			    i_Type = _:DB::TYPE_STRING;

				format(g_Query, DB::BUFFER_SIZE, "ALTER TABLE `%s` ADD COLUMN `%s` TEXT DEFAULT('%s')", g_TableData[table][DB::TABLE_NAME], column, i_DefaultValue);
			}

			default:
			{
				return false;
			}
		}

		new
			DBResult:i_DBResult = db_query(g_Database, g_Query)
		;

		if(i_DBResult)
		{
		    db_free_result(i_DBResult);

			i_ColumnCreated = true;

            DB::SetColumnType(table, column, i_Type);

			DB::ReloadTableInfo(table);

			goto recheck;
		}
	}

	DB::Error("Failed to create the column \"%s\"; this could be because of an invalid column name.", column);
	return false;
}

dbglobal DB::RetrieveKey(table, const column[], { _,Float }:...)
{
 	if (! g_TableData[table][DB::TABLE_NAME][0])
 	{
 	    return DB::INVALID_KEY;
 	}

 	if (strlen(g_TableData[table][DB::TABLE_NAME]) >= DB::MAX_TABLE_NAME)
 	{
 	    return false;
 	}

	if (! column[0])
	{
		return DB::INVALID_KEY;
 	}

	if (strlen(column) >= DB::MAX_COLUMN_NAME)
	{
		return DB::INVALID_KEY;
 	}

	if (! DB::Get())
	{
		return DB::INVALID_KEY;
	}

	for (new i; i < g_ColumnCount[table]; i++)
	{
	    if (! strcmp(g_ColumnNames[table][i], column))
	    {
			switch(DB::GetColumnType(table, g_ColumnNames[table][i]))
			{
			    case DB::TYPE_NUMBER:
			    {
					if (numargs() != 3)
					{
						return DB::INVALID_KEY;
					}
					else
					{
						format(g_Query, DB::BUFFER_SIZE, "SELECT `%s` FROM `%s` WHERE `%s` = '%i'", g_TableData[table][DB::TABLE_KEY], g_TableData[table][DB::TABLE_NAME], column, getarg(2));
			        }
				}

			    case DB::TYPE_FLOAT:
			    {
					if (numargs() != 3)
					{
						return DB::INVALID_KEY;
					}
					else
					{
                    	format(g_Query, DB::BUFFER_SIZE, "SELECT `%s` FROM `%s` WHERE `%s` = '%f'", g_TableData[table][DB::TABLE_KEY], g_TableData[table][DB::TABLE_NAME], column, Float:getarg(2));
					}
				}

			    case DB::TYPE_STRING:
			    {
					if (numargs() != 3)
					{
						return DB::INVALID_KEY;
					}
					else
					{
						new
			            	i_String[DB::MAX_ENTRY_STRING]
						;
						new
							i_Loop = (0x-1)
						;
						while (++i_Loop != 0x100)
						{
						    if(i_Loop < DB::MAX_ENTRY_STRING)
						    {
								i_String[i_Loop] = getarg(2, i_Loop);
							}
						}

						i_String[DB::MAX_ENTRY_STRING - 1] = EOS;

						DB::Escape(i_String);

						format(g_Query, DB::BUFFER_SIZE, "SELECT `%s` FROM `%s` WHERE `%s` = '%s' COLLATE NOCASE", g_TableData[table][DB::TABLE_KEY], g_TableData[table][DB::TABLE_NAME], column, i_String);
					}
     			}

     			default:
     			{
     			    return DB::INVALID_KEY;
     			}
			}

			new
				DBResult:i_DBResult = db_query(g_Database, g_Query)
			;

			if (i_DBResult)
			{
				new
					i_Key = DB::INVALID_KEY
				;
				if (db_num_rows(i_DBResult) >= 1)
				{
					db_get_field(i_DBResult, 0, g_Query, DB::BUFFER_SIZE);
					i_Key = strval(g_Query);
				}

				db_free_result(i_DBResult);

				return i_Key;
			}
			else
			{
	    		return DB::INVALID_KEY;
			}
	    }
	}
	return DB::INVALID_KEY;
}

dbglobal bool:DB::CreateRow(table, const column[], { _,Float }:...)
{
 	if (! g_TableData[table][DB::TABLE_NAME][0])
 	{
 	    return false;
 	}

 	if (strlen(g_TableData[table][DB::TABLE_NAME]) >= DB::MAX_TABLE_NAME)
 	{
 	    return false;
 	}

	if (! column[0])
	{
		return false;
 	}

	if (strlen(column) >= DB::MAX_COLUMN_NAME)
	{
		return false;
 	}

	if (! DB::Get())
	{
		return false;
	}

	for (new i; i < g_ColumnCount[table]; i++)
	{
	    if (! strcmp(g_ColumnNames[table][i], column))
	    {
			switch (DB::GetColumnType(table, g_ColumnNames[table][i]))
			{
			    case DB::TYPE_NUMBER:
			    {
					if (numargs() != 3)
					{
						return false;
					}
					else
					{
						format(g_Query, DB::BUFFER_SIZE, "INSERT INTO `%s`(`%s`) VALUES('%i')", g_TableData[table][DB::TABLE_NAME], column, getarg(2));
			        }
				}

			    case DB::TYPE_FLOAT:
			    {
					if (numargs() != 3)
					{
						return false;
					}
					else
					{
                    	format(g_Query, DB::BUFFER_SIZE, "INSERT INTO `%s`(`%s`) VALUES('%f')", g_TableData[table][DB::TABLE_NAME], column, Float:getarg(2));
			        }
				}

			    case DB::TYPE_STRING:
			    {
					if (numargs() != 3)
					{
						return false;
					}
					else
					{
						new
			            	i_String[DB::MAX_ENTRY_STRING]
						;
						new
							i_Loop = (0x-1)
						;
						while (++i_Loop != 0x100)
						{
						    if (i_Loop < DB::MAX_ENTRY_STRING)
						    {
								i_String[i_Loop] = getarg(2, i_Loop);
							}
						}

						i_String[DB::MAX_ENTRY_STRING - 1] = EOS;

						DB::Escape(i_String);

						format(g_Query, DB::BUFFER_SIZE, "INSERT INTO `%s`(`%s`) VALUES('%s')", g_TableData[table][DB::TABLE_NAME], column, i_String);
			        }
     			}

     			default:
     			{
     			    return false;
     			}
			}

			new
				DBResult:i_DBResult = db_query(g_Database, g_Query)
			;

			if (i_DBResult)
			{
				db_free_result(i_DBResult);
				return true;
			}
			else
			{
			    return false;
			}
	    }
	}
	return false;
}

dbglobal DB::DeleteRow(table, key)
{
 	if (! g_TableData[table][DB::TABLE_NAME][0])
 	{
 	    return false;
 	}

 	if (strlen(g_TableData[table][DB::TABLE_NAME]) >= DB::MAX_TABLE_NAME)
 	{
 	    return false;
 	}

	if (DB::RetrieveKey(table, g_TableData[table][DB::TABLE_KEY], key) == DB::INVALID_KEY)
	{
		return false;
	}

	if (! DB::Get())
	{
		return false;
	}

	format(g_Query, DB::BUFFER_SIZE, "DELETE FROM `%s` WHERE `%s`='%i'", g_TableData[table][DB::TABLE_NAME], g_TableData[table][DB::TABLE_KEY], key);

	new
		DBResult:i_DBResult = db_query(g_Database, g_Query)
	;

	if (i_DBResult)
	{
	    db_free_result(i_DBResult);

		if (DB::RetrieveKey(table, g_TableData[table][DB::TABLE_KEY], key) != DB::INVALID_KEY)
		{
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}
}

dbglobal DB::GetIntEntry(table, key, const column[])
{
 	if (! g_TableData[table][DB::TABLE_NAME][0])
 	{
 	    return 0;
 	}

 	if (strlen(g_TableData[table][DB::TABLE_NAME]) >= DB::MAX_TABLE_NAME)
 	{
 	    return 0;
 	}

	if (! column[0])
	{
		return 0;
 	}

	if (strlen(column) >= DB::MAX_COLUMN_NAME)
	{
		return 0;
 	}

	if (! DB::Get())
	{
		return 0;
	}

	for (new i; i < g_ColumnCount[table]; i++)
	{
	    if (! strcmp(g_ColumnNames[table][i], column))
	    {
			switch (DB::GetColumnType(table, g_ColumnNames[table][i]))
			{
			    case DB::TYPE_NUMBER:
			    {
					format(g_Query, DB::BUFFER_SIZE, "SELECT `%s` FROM `%s` WHERE `%s`='%d'", column, g_TableData[table][DB::TABLE_NAME], g_TableData[table][DB::TABLE_KEY], key);
				}

     			default:
     			{
     			    return 0;
     			}
			}

			new
				DBResult:i_DBResult = db_query(g_Database, g_Query),
				i_Value = 0
			;

			if (i_DBResult)
			{
				if (db_num_rows(i_DBResult) == 1)
				{
					i_Value = db_get_field_int(i_DBResult, 0);
				}

				db_free_result(i_DBResult);
				
				return i_Value;
			}
			else
			{
			    return 0;
			}
	    }
	}
	return 0;
}

dbglobal Float:DB::GetFloatEntry(table, key, const column[])
{
 	if (! g_TableData[table][DB::TABLE_NAME][0])
 	{
 	    return 0.0;
 	}

 	if (strlen(g_TableData[table][DB::TABLE_NAME]) >= DB::MAX_TABLE_NAME)
 	{
 	    return 0.0;
 	}

	if (! column[0])
	{
		return 0.0;
 	}

	if (strlen(column) >= DB::MAX_COLUMN_NAME)
	{
		return 0.0;
 	}

	if (! DB::Get())
	{
		return 0.0;
	}

	for (new i; i < g_ColumnCount[table]; i++)
	{
	    if (! strcmp(g_ColumnNames[table][i], column))
	    {
			switch (DB::GetColumnType(table, g_ColumnNames[table][i]))
			{
			    case DB::TYPE_FLOAT:
			    {
					format(g_Query, DB::BUFFER_SIZE, "SELECT `%s` FROM `%s` WHERE `%s`='%d'", column, g_TableData[table][DB::TABLE_NAME], g_TableData[table][DB::TABLE_KEY], key);
				}

     			default:
     			{
     			    return 0.0;
     			}
			}

			new
				DBResult:i_DBResult = db_query(g_Database, g_Query),
				Float:i_Value = 0.0
			;

			if (i_DBResult)
			{
				if (db_num_rows(i_DBResult) == 1)
				{
					i_Value = db_get_field_float(i_DBResult, 0);
				}

				db_free_result(i_DBResult);

				return i_Value;
			}
			else
			{
			    return 0.0;
			}
	    }
	}
	return 0.0;
}

dbglobal bool:DB::GetStringEntry(table, key, const column[], str[], len = sizeof(str))
{
 	if (! g_TableData[table][DB::TABLE_NAME][0])
 	{
 	    return false;
 	}

 	if (strlen(g_TableData[table][DB::TABLE_NAME]) >= DB::MAX_TABLE_NAME)
 	{
 	    return false;
 	}

	if (! column[0])
	{
 	    return false;
 	}

	if (strlen(column) >= DB::MAX_COLUMN_NAME)
	{
 	    return false;
 	}

	if (! DB::Get())
	{
 	    return false;
	}

	for (new i; i < g_ColumnCount[table]; i++)
	{
	    if (! strcmp(g_ColumnNames[table][i], column))
	    {
			switch (DB::GetColumnType(table, g_ColumnNames[table][i]))
			{
			    case DB::TYPE_STRING:
			    {
					format(g_Query, DB::BUFFER_SIZE, "SELECT `%s` FROM `%s` WHERE `%s`='%d'", column, g_TableData[table][DB::TABLE_NAME], g_TableData[table][DB::TABLE_KEY], key);
				}

     			default:
     			{
			 	    return false;
     			}
			}

			new
				DBResult:i_DBResult = db_query(g_Database, g_Query)
			;

			if (i_DBResult)
			{
				if (db_num_rows(i_DBResult) == 1)
				{
					db_get_field(i_DBResult, 0, str, len);
				}
				else
				{
					str[0] = EOS;
				}

				db_free_result(i_DBResult);

				if (str[0])
				{
					return true;
				}
			 	return false;
			}
			else
			{
			 	return false;
			}
	    }
	}
    return false;
}

dbglobal bool:DB::SetIntEntry(table, key, const column[], value)
{
 	if (! g_TableData[table][DB::TABLE_NAME][0])
 	{
 	    return false;
 	}

 	if (strlen(g_TableData[table][DB::TABLE_NAME]) >= DB::MAX_TABLE_NAME)
 	{
 	    return false;
 	}

	if (! column[0])
	{
		return false;
 	}

	if (strlen(column) >= DB::MAX_COLUMN_NAME)
	{
		return false;
 	}

	if (! DB::Get())
	{
		return false;
	}

	for (new i; i < g_ColumnCount[table]; i++)
	{
	    if (! strcmp(g_ColumnNames[table][i], column))
	    {
			switch (DB::GetColumnType(table, g_ColumnNames[table][i]))
			{
			    case DB::TYPE_NUMBER:
			    {
					format(g_Query, DB::BUFFER_SIZE, "UPDATE `%s` SET `%s`='%d' WHERE `%s`='%d'", g_TableData[table][DB::TABLE_NAME], column, value, g_TableData[table][DB::TABLE_KEY], key);
				}

     			default:
     			{
     			    return false;
     			}
			}

			new
				DBResult:i_DBResult = db_query(g_Database, g_Query)
			;

			if (i_DBResult)
			{
				db_free_result(i_DBResult);

				return true;
			}
			else
			{
			    return false;
			}
	    }
	}
	return false;
}

dbglobal bool:DB::SetFloatEntry(table, key, const column[], Float:value)
{
 	if (! g_TableData[table][DB::TABLE_NAME][0])
 	{
 	    return false;
 	}

 	if (strlen(g_TableData[table][DB::TABLE_NAME]) >= DB::MAX_TABLE_NAME)
 	{
 	    return false;
 	}

	if (! column[0])
	{
		return false;
 	}

	if (strlen(column) >= DB::MAX_COLUMN_NAME)
	{
		return false;
 	}

	if (! DB::Get())
	{
		return false;
	}

	for (new i; i < g_ColumnCount[table]; i++)
	{
	    if (! strcmp(g_ColumnNames[table][i], column))
	    {
			switch (DB::GetColumnType(table, g_ColumnNames[table][i]))
			{
			    case DB::TYPE_FLOAT:
			    {
					format(g_Query, DB::BUFFER_SIZE, "UPDATE `%s` SET `%s`='%f' WHERE `%s`='%d'", g_TableData[table][DB::TABLE_NAME], column, value, g_TableData[table][DB::TABLE_KEY], key);
				}

     			default:
     			{
     			    return false;
     			}
			}

			new
				DBResult:i_DBResult = db_query(g_Database, g_Query)
			;

			if (i_DBResult)
			{
				db_free_result(i_DBResult);

				return true;
			}
			else
			{
			    return false;
			}
	    }
	}
	return false;
}

dbglobal bool:DB::SetStringEntry(table, key, const column[], value[], len = sizeof(value))
{
 	if (! g_TableData[table][DB::TABLE_NAME][0])
 	{
 	    return false;
 	}

 	if (strlen(g_TableData[table][DB::TABLE_NAME]) >= DB::MAX_TABLE_NAME)
 	{
 	    return false;
 	}

	if (! column[0])
	{
		return false;
 	}

	if (strlen(column) >= DB::MAX_COLUMN_NAME)
	{
		return false;
 	}

	if (! DB::Get())
	{
		return false;
	}

	for (new i; i < g_ColumnCount[table]; i++)
	{
	    if (! strcmp(g_ColumnNames[table][i], column))
	    {
			switch (DB::GetColumnType(table, g_ColumnNames[table][i]))
			{
			    case DB::TYPE_STRING:
			    {
					DB::Escape(value, _, len);
					format(g_Query, DB::BUFFER_SIZE, "UPDATE `%s` SET `%s`='%s' WHERE `%s`='%d'", g_TableData[table][DB::TABLE_NAME], column, value, g_TableData[table][DB::TABLE_KEY], key);
				}

     			default:
     			{
     			    return false;
     			}
			}

			new
				DBResult:i_DBResult = db_query(g_Database, g_Query)
			;

			if (i_DBResult)
			{
				db_free_result(i_DBResult);

				return true;
			}
			else
			{
			    return false;
			}
	    }
	}
	return false;
}

dbglobal DBResult:DB::Query(query[], bool:keepresult = false)
{
	if (! DB::Get())
	{
		return DBResult:0;
	}

	if (! query[0])
	{
	    return DBResult:0;
	}

	new
		DBResult:i_DBResult = db_query(g_Database, query)
	;

	if(! keepresult)
	{
		SetTimerEx("DB_FreeResult", 2000, false, "i", _:i_DBResult);
	}

	if (i_DBResult)
	{
		return i_DBResult;
	}
	return DBResult:0;
}

forward DB_FreeResult(DBResult:handle);
public	DB_FreeResult(DBResult:handle)
{
	db_free_result(handle);

	return 1;
}

dbglobal DB::CountTables()
{
	if (! g_Initialized)
	{
	    return 0;
 	}

	return g_TableCount;
}

dbglobal DB::CountColumns(table)
{
	if (! g_Initialized)
	{
	    return 0;
 	}

 	if (! g_TableData[table][DB::TABLE_NAME][0])
 	{
 	    return 0;
 	}

	return g_ColumnCount[table];
}

dbglobal DB::CountRows(table)
{
	if (! DB::Get())
	{
		return 0;
	}

 	if (! g_TableData[table][DB::TABLE_NAME][0])
 	{
 	    return 0;
 	}

	format(g_Query, sizeof(g_Query), "SELECT * FROM `%s` WHERE 1", g_TableData[table][DB::TABLE_NAME], g_TableData[table][DB::TABLE_KEY]);

	new
		DBResult:i_DBResult = db_query(g_Database, g_Query),
		i_Rows = 0
	;
	if(i_DBResult)
	{
	    i_Rows = db_num_rows(i_DBResult);

	    db_free_result(i_DBResult);
	}

	return i_Rows;
}

#if defined WP_Hash

	//declare DB::Hash as WP_Hash only if the plugin is loaded before the include
    dbglobal DB::Hash(buffer[], len = sizeof(buffer), const str[]) //SAMP 0.3.7 R1 package
	{
		return WP_Hash(buffer, len, str);
	}

#elseif defined SHA256_PassHash

    dbglobal DB::Hash(buffer[], len = sizeof(buffer), str[], salt[] = "78sdjs86d2h") //SAMP 0.3.7 R1 package
	{
		return SHA256_PassHash(str, salt, buffer, len);
	}

#else

	dbstatic DB::chrfind(needle, haystack[], start = 0) // Y_Less
	{
		while(haystack[start])
		{
			if(haystack[start++] == needle)
			{
				return start - 1;
			}
		}
		return -1;
	}

	dbglobal DB::Hash(buffer[], len = sizeof(buffer), const str[]) // Y_Less's dad (JSC)
	{
		static charset[] = \"4YLi6pOX)Mudvbc_IFVB/8HZ\2r(fGjaN0oU9C1Wywnq*smKQRxJDhkAS|53EzglT7tPe";
		static cells = 69;

		new j = strlen(buffer);
		new s = j;
		new t = 0;
		new m;

		for(new i = 0; i < len || i < j; i++)
		{
			m = i % len;
			t = (i >= j) ? charset[(7 * i) % cells] : buffer[i];
			s = (s + DB::chrfind(t, charset) + 1) % cells;
			bufferhash[m] = charset[(s + bufferhash[m]) % cells];
		}
		bufferhash[len] = '\0';
	}

#endif

dbglobal DB::Escape(str[], ichar = '\'', len = sizeof(str))
{
	new
		i_Length = strlen(str),
		i_Insert[2]
	;

	i_Insert[0] = ichar;

	for (new i = 0; i <= i_Length; i++)
	{
		if (str[i] == ichar)
		{
			if (i > i_Length - 1)
			{
				str[i] = EOS;

				break;
			}
			else
			{
				if (i_Length >= len - 1)
				{
					str[i_Length - 1] = EOS;

					--i_Length;
				}

				strins(str, i_Insert, i, len);

				++i_Length;
				++i;
			}
		}
	}

	return true;
}

//undefine the private include constants
#undef dbstatic
#undef dbglobal

#undef DB_Notice
#undef DB_Warning
#undef DB_Error

#undef DB_MAX_DATABASE_NAME

#undef DB_MAX_COLUMNS
#undef DB_MAX_COLUMN_NAME

#undef DB_MAX_ENTRY_STRING

#undef DB_MAX_TABLES
#undef DB_MAX_TABLE_NAME

#undef DB_BUFFER_SIZE

#undef DB_ALIVE_TIME

#undef DB_TIMEOUT_CHECK

#undef DB_SYNCHRONOUS

#undef DEFAULT_DATABASE_NAME

#undef INVALID_DB
