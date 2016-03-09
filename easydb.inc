#if defined easydb_included
	#endinput
#endif
#define easydb_included

/*
* Easydb.inc (SQLite) - Build 7
* Easy database builder is a library allowing you to create SQLite database without any SQL knowledge requirements.
*
* Credits:
* - Creating this include
* - Y_Less for string argument functions
* - Slice for BUD include as refrence
*/

/*
native bool:DB::Open(name[]);
native bool:DB::Close();
native DB::VerifyTable(const table[], const key[] = "ROW_ID");
native bool:DB::VerifyColumn(table, const column[], DBType:type, { _,Float }:...);
native DB::RetrieveKey(table, keys[] = "", size = sizeof (keys), const column[], { _,Float }:...);
native bool:DB::CreateRow(table, const column[], { _,Float }:...);
native bool:DB::DeleteRow(table, key);
native DB::GetIntEntry(table, key, const column[]);
native Float:DB::GetFloatEntry(table, key, const column[]);
native bool:DB::GetStringEntry(table, key, const column[], get[], len = sizeof(get));
native bool:DB::SetIntEntry(table, key, const column[], value);
native bool:DB::SetFloatEntry(table, key, const column[], Float:value);
native bool:DB::SetStringEntry(table, key, const column[], const value[]);
native DBResult:DB::Query(query[], bool:keepresult = false);
native bool:DB::MultiSet(table, key, const fmat[], {_, Float}:...);
native bool:DB::MultiGet(table, key, const fmat[], {_, Float}:...);
native DB::CountColumns(table);
native DB::CountRows(table);
native DB::SortFloatData(table, const column[], keys[], Float:values[], bool:decending = true, const conditions[] = "", len = sizeof(keys));
native DB::SortIntData(table, const column[], keys[], values[], bool:decending = true, const conditions[] = "", len = sizeof(keys));
*/

//naming ease
#define DB::      					DB_
#define dbstatic    				stock static
#define dbglobal     				stock

//notification defines
#define DB_Notice(%0)       		(printf("[easydb.inc] - NOTICE: " %0))
#define DB_Warning(%0)      		(printf("[easydb.inc] - WARNING: " %0))
#define DB_Error(%0)        		(printf("[easydb.inc] - ERROR: " %0))

//constants (you can define them before inclution also)
#if ! defined DB_MAX_NAME
	#define DB_MAX_NAME (45)
#endif

#if ! defined DB_MAX_ALIVETIME
	#define DB_MAX_ALIVETIME (3000)
#endif

#if ! defined DB_MAX_ENTRY
	#define DB_MAX_ENTRY (1024)
#endif

#if ! defined DB_MAX_QUERY
	#define DB_MAX_QUERY (3024)
#endif

#if ! defined DB_MAX_TABLES
	#define DB_MAX_TABLES (10)
#endif

//the defualt DB name (no need to change, you can do it in DB::Open)
//the deafult name is there in case the name isn't appropriate or invalid when inputed in DB::Open
#define DEFAULT_DATABASE_NAME  		"server.db"

//invalid defines
#define DB_INVALID           		(DB:0)
#define DB_INVALID_KEY      		(0)
#define DB_INVALID_TABLE    		(-1)

//column types enum
enum DBType
{
	DB::TYPE_NUMBER,
	DB::TYPE_FLOAT,
	DB::TYPE_STRING
};

//declaring variables
dbstatic g_DatabaseName[DB::MAX_NAME];
dbstatic DB:g_Database;
dbstatic g_DatabaseTimer;
dbstatic bool:g_Initialized;

dbstatic g_TableName[DB::MAX_TABLES][DB::MAX_NAME];
dbstatic g_TableKey[DB::MAX_TABLES][DB::MAX_NAME];
dbstatic g_TableCount;

dbstatic g_Query[DB::MAX_QUERY];

//Internal functions
dbstatic bool:DB::Get()
{
    if (! g_Initialized)
    {
        return false;
    }

	if (! g_DatabaseTimer)
	{
		g_Database = db_open(g_DatabaseName);

		if (g_Database == DB_INVALID)
		{
			DB::Error("Unable to open the database \"%s\".", g_DatabaseName);

			return false;
		}
		else
		{
			db_query(g_Database, "PRAGMA synchronous = 0");
		}
	}
	else
	{
		KillTimer(g_DatabaseTimer);
	}

	g_DatabaseTimer = SetTimer("OnDBClose", DB::MAX_ALIVETIME, false);

	return true;
}

forward OnDBClose();
public  OnDBClose()
{
	g_DatabaseTimer = 0;

	db_close(g_Database);
	g_Database = DB_INVALID;
}

forward DB::_skip_no_response();
public 	DB::_skip_no_response()
{
	strcat("", "", 0);
}

dbstatic DB::getstringarg(dest[], arg, len = sizeof (dest))
{
    // Get the address of the previous function's stack.  First get the index of
    // the argument required.
    #emit LOAD.S.pri arg

    // Then convert that number to bytes from cells.
    #emit SMUL.C     4
    // Get the previous function's frame.  Stored in variable 0 (in the current
    // frame).  Parameters are FRM+n+12, locals are FRM-n, previous frame is
    // FRM+0, return address is FRM+4, parameter count is FRM+8.  We could add
    // checks that "arg * 4 < *(*(FRM + 0) + 8)", for the previous frame parameter
    // count (in C pointer speak).
    #emit LOAD.S.alt 0

    // Add the frame pointer to the argument offset in bytes.
    #emit ADD

    // Add 12 to skip over the function header.
    #emit ADD.C      12

    // Load the address stored in the specified address.
    #emit LOAD.I

    // Push the length for "strcat".
    #emit PUSH.S     len

    // Push the address we just determined was the source.
    #emit PUSH.pri

    // Load the address of the destination.
    #emit LOAD.S.alt dest

    // Blank the first cell so "strcat" behaves like "strcpy".
    #emit CONST.pri  0

    // Store the loaded number 0 to the loaded address.
    #emit STOR.I

    // Push the loaded address.
    #emit PUSH.alt

    // Push the number of parameters passed (in bytes) to the function.
    #emit PUSH.C     12

    // Call the function.
    #emit SYSREQ.C   strcat

    // Restore the stack to its level before we called this native.
    #emit STACK      16
}

dbstatic DB::setstringarg(arg, const value[], len = sizeof (value))
{
	new address;

	// Get the address of the previous function's stack.  First get the index of
    // the argument required.
    #emit LOAD.S.pri arg

    // Then convert that number to bytes from cells.
    #emit SMUL.C     4

    // Get the previous function's frame.
	#emit LOAD.S.alt 0

	// Add the frame pointer to the argument offset in bytes.
    #emit ADD

    // Add 12 to skip over the function header.
    #emit ADD.C      12

    // Load the address stored in the specified address.
    #emit LOAD.I
	#emit STOR.S.PRI address

	// Push the length (last argument first)
	#emit PUSH.S     len

	// Push the new value (source) value
	#emit PUSH.S     value

	// Blank out the first cell of the argument
	#emit CONST.pri  0
	#emit SREF.S.pri address

	// Push the destination
	#emit PUSH.S     address

	// Push the number of parameters passed (in bytes) to the function.
	#emit PUSH.C     12

	// Call the function.
	#emit SYSREQ.C   strcat

	// Restore the stack to its level before we called this native.
	#emit STACK      16
}

//Main functions
dbglobal bool:DB::Open(name[])
{
	if (g_Database != DB::INVALID)
	{
		DB::Notice("Initialization aborted; Database is already initialized.");

		return true;
	}

	if (! name[0])
	{
		DB::Warning("The database name was not specified; default name will be applied (\""DEFAULT_DATABASE_NAME"\").");
		g_DatabaseName = DEFAULT_DATABASE_NAME;
	}
	else
	{
	    g_DatabaseName[0] = EOS;
	    strcat(g_DatabaseName, name);
	}

	if (! fexist(name))
	{
		DB::Notice("The database \"%s\" doesn't exist; it will be created.", g_DatabaseName);

        g_Database = db_open(name);
  		if (! g_Database)
		{
			DB::Error("The database \"%s\" wasn't created, the include have unloaded.", g_DatabaseName);
			return false;
		}
		else
		{
		    db_close(g_Database);
		}
	}

	for (new i; i < DB::MAX_TABLES; i++)
	{
	    g_TableName[i][0] = EOS;
	    g_TableKey[i][0] = EOS;
	}
	g_TableCount = 0;

	g_Initialized = true;

	print("[easydb.inc] Include loaded (Build 7).");
	return true;
}

dbglobal bool:DB::Close()
{
	if (g_Database == DB::INVALID)
	{
	    return false;
 	}

	if (g_DatabaseTimer != -1)
	{
		KillTimer(g_DatabaseTimer);

		OnDBClose();
	}

	g_Initialized = false;

	return true;
}

dbglobal DB::VerifyTable(const table[], const key[] = "ROW_ID")
{
	for (new i = 0; i < g_TableCount; i++)
	{
		if (g_TableName[i][0] && ! strcmp(g_TableName[i], table))
		{
			return i;
		}
	}

    if (! DB::Get())
	{
		return DB::INVALID_TABLE;
	}

    g_Query[0] = EOS;
    strcat(g_Query, "SELECT * FROM `");
    strcat(g_Query, table);
	strcat(g_Query, "` LIMIT 1");
	new DBResult:result = db_query(g_Database, g_Query);
	if (! result)
	{
	    db_free_result(result);

	    DB::Notice("The table \"%s\" doesn't exist; attempting to create it.", table);

	    g_Query[0] = EOS;
	    strcat(g_Query, "CREATE TABLE `");
	    strcat(g_Query, table);
		strcat(g_Query, "` (`");
	    strcat(g_Query, key);
		strcat(g_Query, "` INTEGER PRIMARY KEY)");
		db_free_result(db_query(g_Database, g_Query));
	}

    g_TableName[g_TableCount][0] = EOS;
	strcat(g_TableName[g_TableCount], table);

    g_TableKey[g_TableCount][0] = EOS;
	strcat(g_TableKey[g_TableCount], key);

	g_TableCount++;
	return (g_TableCount - 1);
}

dbglobal bool:DB::VerifyColumn(table, const column[], DBType:type, { _,Float }:...)
{
    g_Query[0] = EOS;
    strcat(g_Query, "SELECT `");
    strcat(g_Query, column);
    strcat(g_Query, "` FROM `");
    strcat(g_Query, g_TableName[table]);
    strcat(g_Query, "` LIMIT 1");
	new DBResult:result = db_query(g_Database, g_Query);
	if (! result)
	{
		DB::Notice("The column \"%s\" doesn't exist; attempting to create it.", column);

		if (! DB::Get())
		{
			return false;
		}

		switch (type)
		{
			case DB::TYPE_NUMBER:
			{
				new default_value = 0;
				if (numargs() == 4)
				{
					default_value = getarg(3);
				}

				format(g_Query, DB::MAX_QUERY, "ALTER TABLE `%s` ADD COLUMN `%s` INTEGER DEFAULT(%d)", g_TableName[table], column, default_value);
			}

			case DB::TYPE_FLOAT:
			{
				new Float:default_value = 0.0;
				if (numargs() != 4)
				{
					default_value = Float:getarg(3);
				}

				format(g_Query, DB::MAX_QUERY, "ALTER TABLE `%s` ADD COLUMN `%s` REAL DEFAULT(%f)", g_TableName[table], column, default_value);
			}

			case DB::TYPE_STRING:
			{
				new default_value[DB::MAX_ENTRY * 2] = "";
				if (numargs() == 4)
				{
					DB::getstringarg(default_value, 3);
				}

				format(g_Query, DB::MAX_QUERY, "ALTER TABLE `%s` ADD COLUMN `%s` TEXT DEFAULT('%q')", g_TableName[table], column, default_value);
			}

			default:
			{
				return false;
			}
		}

		result = db_query(g_Database, g_Query);

		if (result)
		{
		    db_free_result(result);
		    return true;
		}
		else
		{
		    DB::Error("Failed to create the column \"%s\"; this could be because of an invalid column name.", column);
		    return false;
		}
	}

	return true;
}

dbglobal DB::RetrieveKey(table, keys[] = "", size = sizeof (keys), const column[], { _,Float }:...)
{
	if (! DB::Get())
	{
		return DB::INVALID_KEY;
	}

    if (numargs() != 5)
	{
		return DB::INVALID_KEY;
	}

	new args[DB::MAX_ENTRY * 2];
	DB::getstringarg(args, 4);

	format(g_Query, DB::MAX_QUERY, "SELECT `%s` FROM `%s` WHERE `%s` = '%q'", g_TableKey[table], g_TableName[table], column, args);

	new DBResult:result = db_query(g_Database, g_Query);
	if (result)
	{
	    if (! size)
	    {
	        new key = db_get_field_int(result, 0);

			db_free_result(result);
		
		    return key;
	    }

		new rows = db_num_rows(result);
		
		for (new i; i < size; i++)
		{
		    if (i < rows)
		    {
				keys[i] = db_get_field_int(result, 0);

				db_next_row(result);
			}
			else
			{
			    keys[i] = DB::INVALID_KEY;
			}
		}

		db_free_result(result);

		return keys[0];
	}

	return DB::INVALID_KEY;
}

dbglobal bool:DB::CreateRow(table, const column[], { _,Float }:...)
{
	if (! DB::Get())
	{
		return false;
	}

    if (numargs() != 3)
	{
		return false;
	}

	new args[DB::MAX_ENTRY * 2];
	DB::getstringarg(args, 2);

    format(g_Query, DB::MAX_QUERY, "INSERT INTO `%s`(`%s`) VALUES('%q')", g_TableName[table], column, args);

	new DBResult:result = db_query(g_Database, g_Query);
	if (result)
	{
		db_free_result(result);
		return true;
	}
	return false;
}

dbglobal bool:DB::DeleteRow(table, key)
{
	if (! DB::Get())
	{
		return false;
	}

	format(g_Query, DB::MAX_QUERY, "DELETE FROM `%s` WHERE `%s` = '%i'", g_TableName[table], g_TableKey[table], key);

	new DBResult:result = db_query(g_Database, g_Query);
	if (result)
	{
		db_free_result(result);
		return true;
	}
	return false;
}

dbglobal DB::GetIntEntry(table, key, const column[])
{
	if (! DB::Get())
	{
		return 0;
	}

	format(g_Query, DB::MAX_QUERY, "SELECT `%s` FROM `%s` WHERE `%s` = '%d'", column, g_TableName[table], g_TableKey[table], key);

    new DBResult:result = db_query(g_Database, g_Query);
	if (result)
	{
	    new value;
	    
	    if (db_num_rows(result))
	    {
	    	value = db_get_field_int(result, 0);
		}

		db_free_result(result);

		return value;
	}

	return 0;
}

dbglobal Float:DB::GetFloatEntry(table, key, const column[])
{
	if (! DB::Get())
	{
		return 0.0;
	}

	format(g_Query, DB::MAX_QUERY, "SELECT `%s` FROM `%s` WHERE `%s` = '%d'", column, g_TableName[table], g_TableKey[table], key);

    new DBResult:result = db_query(g_Database, g_Query);
	if (result)
	{
	    new Float:value;

	    if (db_num_rows(result))
	    {
			value = db_get_field_float(result, 0);
		}

		db_free_result(result);

		return value;
	}

	return 0.0;
}

dbglobal bool:DB::GetStringEntry(table, key, const column[], get[], len = sizeof(get))
{
	if (! DB::Get())
	{
		return false;
	}

	format(g_Query, DB::MAX_QUERY, "SELECT `%s` FROM `%s` WHERE `%s` = '%d'", column, g_TableName[table], g_TableKey[table], key);

    new DBResult:result = db_query(g_Database, g_Query);
	if (result)
	{
	    if (db_num_rows(result))
	    {
			db_get_field(result, 0, get, len);
		}

		db_free_result(result);

		return true;
	}

    get[0] = EOS;
	return false;
}

dbglobal bool:DB::SetIntEntry(table, key, const column[], value)
{
	if (! DB::Get())
	{
		return false;
	}

	format(g_Query, DB::MAX_QUERY, "UPDATE `%s` SET `%s` = '%d' WHERE `%s` = '%d'", g_TableName[table], column, value, g_TableKey[table], key);

	new DBResult:result = db_query(g_Database, g_Query);
	if (result)
	{
		db_free_result(result);

		return true;
	}

	return false;
}

dbglobal bool:DB::SetFloatEntry(table, key, const column[], Float:value)
{
	if (! DB::Get())
	{
		return false;
	}

	format(g_Query, DB::MAX_QUERY, "UPDATE `%s` SET `%s` = '%f' WHERE `%s` = '%d'", g_TableName[table], column, value, g_TableKey[table], key);

	new DBResult:result = db_query(g_Database, g_Query);
	if (result)
	{
		db_free_result(result);

		return true;
	}

	return false;
}

dbglobal bool:DB::SetStringEntry(table, key, const column[], const value[])
{
	if (! DB::Get())
	{
		return false;
	}

	format(g_Query, DB::MAX_QUERY, "UPDATE `%s` SET `%s` = '%q' WHERE `%s` = '%d'", g_TableName[table], column, value, g_TableKey[table], key);

	new DBResult:result = db_query(g_Database, g_Query);
	if (result)
	{
		db_free_result(result);

		return true;
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

	new DBResult:result = db_query(g_Database, query);

	if (! keepresult)
	{
		SetTimerEx("OnDBFreeResult", DB::MAX_ALIVETIME, false, "i", _:result);
	}

	return result;
}

forward OnDBFreeResult(DBResult:handle);
public	OnDBFreeResult(DBResult:handle)
{
	db_free_result(handle);
}

dbglobal bool:DB::MultiSet(table, key, const fmat[], {_, Float}:...)
{
	if (! DB::Get())
	{
		return false;
	}

    g_Query[0] = EOS;

	for (new i, j = strlen(fmat); i < j; i++)
	{
		switch (fmat[i])
		{
			case 'i', 'd', 'f', 's':
			{
			}
			default:
			{
				return false;
			}
		}
	}

	new column[DB::MAX_NAME];
	new column_arg;
	new value[DB::MAX_ENTRY * 2];

	g_Query = "UPDATE `";
	strcat(g_Query, g_TableName[table]);
	strcat(g_Query, "` SET ");

	for (new i, j = strlen(fmat); i < j; i++)
	{
		column_arg = 3 + i * 2;

		DB::getstringarg(column, column_arg);

		++column_arg;

		strcat(g_Query, "`");
		strcat(g_Query, column);
		strcat(g_Query, "` = '");

		switch (fmat[i])
		{
			case 'i', 'd':
			{
				valstr(value, getarg(column_arg));
			}
			case 'f':
			{
				format(value, sizeof(value), "%f", Float:getarg(column_arg));
			}
			case 's':
			{
				DB::getstringarg(value, column_arg, DB::MAX_ENTRY);
			}
		}

		strcat(g_Query, value);
		strcat(g_Query, "'");

		if (i < j - 1)
		{
			strcat(g_Query, ", ");
		}
	}

	valstr(value, key);

	strcat(g_Query, " WHERE `");
	strcat(g_Query, g_TableKey[table]);
	strcat(g_Query, "` = '");
	strcat(g_Query, value);
	strcat(g_Query, "'");

	db_free_result(db_query(g_Database, g_Query));
	return true;
}

dbglobal bool:DB::MultiGet(table, key, const fmat[], {_, Float}:...)
{
	if (! DB::Get())
	{
		return false;
	}
	
	new column[DB::MAX_NAME];
	new column_arg;

	g_Query = "SELECT `";

	for (new i, j = strlen(fmat); i < j; i++)
	{
		column_arg = 3 + i * 2;

		DB::getstringarg(column, column_arg);

		strcat(g_Query, column);

		if (i < j - 1)
		{
			strcat(g_Query, "`, `");
		}
	}

	strcat(g_Query, "` FROM `");
	strcat(g_Query, g_TableName[table]);
	strcat(g_Query, "` WHERE `");
	strcat(g_Query, g_TableKey[table]);
	strcat(g_Query, "` = '");

	new str_rowid[11];
	new DBResult:result;

	valstr(str_rowid, key);
	strcat(g_Query, str_rowid);
	strcat(g_Query, "'");

	result = db_query(g_Database, g_Query);
	if (result)
	{
	    if (! db_num_rows(result))
	    {
			db_free_result(result);
			return false;
	    }
	    
	    new str[DB::MAX_ENTRY];
		for (new i, j = db_num_fields(result); i < j; i++)
		{
			switch (fmat[i])
			{
				case 'i', 'd':
				{
					setarg((3 + i * 2) + 1, .value = db_get_field_int(result, i));
				}
				case 'f':
				{
					setarg((3 + i * 2) + 1, .value = _:db_get_field_float(result, i));
				}
				case 's':
				{
					db_get_field(result, i, str, DB::MAX_ENTRY);
					DB::setstringarg((3 + i * 2) + 1, str);
   				}
			}
		}

		db_free_result(result);
		return true;
	}

	return false;
}

dbglobal DB::CountColumns(table)
{
	if (! DB::Get())
	{
		return false;
	}

    g_Query[0] = EOS;
 	strcat(g_Query, "SELECT * FROM `");
	strcat(g_Query, g_TableName[table]);
	strcat(g_Query, "` WHERE 1");

	new DB:Result:result = db_query(g_Database, g_Query);
	if (result)
	{
		new fields = db_num_fields(result);

		db_free_result(result);

		return fields;
	}

	return 0;
}

dbglobal DB::CountRows(table)
{
	if (! DB::Get())
	{
		return 0;
	}

	g_Query[0] = EOS;
 	strcat(g_Query, "SELECT * FROM `");
	strcat(g_Query, g_TableName[table]);
	strcat(g_Query, "` WHERE 1");

	new DBResult:result = db_query(g_Database, g_Query);
	if (result)
	{
		new rows = db_num_rows(result);

		db_free_result(result);

		return rows;
	}

	return 0;
}

dbglobal DB::SortIntData(table, const column[], keys[], values[], bool:decending = true, const conditions[] = "", len = sizeof(keys))
{
	if (! DB::Get())
	{
		return 0;
	}

	format(g_Query, DB::MAX_QUERY, "SELECT `%s`, `%s` FROM `%s` %s ORDER BY `%s` %s LIMIT %d", g_TableKey[table], column, g_TableName[table], conditions, column, (decending == true) ? ("DESC") : ("ASC"), len);

	new DBResult:result = db_query(g_Database, g_Query);
	if (result)
	{
		if (db_num_rows(result))
		{
			new index = 0;

			do
			{
				keys[index] = db_get_field_int(result, 0);
				values[index] = db_get_field_int(result, 1);

				++index;
			}
			while
				(db_next_row(result) && index < len);

			db_free_result(result);
			return index;
		}

		db_free_result(result);
		return 0;
	}

	return 0;
}

dbglobal DB::SortFloatData(table, const column[], keys[], Float:values[], bool:decending = true, const conditions[] = "", len = sizeof(keys))
{
	if (! DB::Get())
	{
		return 0;
	}

	format(g_Query, DB::MAX_QUERY, "SELECT `%s`, `%s` FROM `%s` %s ORDER BY `%s` %s LIMIT %d", g_TableKey[table], column, g_TableName[table], conditions, column, (decending == true) ? ("DESC") : ("ASC"), len);

	new DBResult:result = db_query(g_Database, g_Query);
	if (result)
	{
		if (db_num_rows(result))
		{
			new index = 0;

			do
			{
				keys[index] = db_get_field_int(result, 0);
				values[index] = db_get_field_float(result, 1);

				++index;
			}
			while
				(db_next_row(result) && index < len);

			db_free_result(result);
			return index;
		}

		db_free_result(result);
		return 0;
	}

	return 0;
}

dbglobal DB::GetRowsPoolSize(table)
{
	new key[1];
	new value[1];
    DB::SortIntData(table, g_TableKey[table], key, value);

	return key[0];
}

#define DB_GetHighestRegisteredKey DB_GetRowsPoolSize

//undefine the private include constants
#undef dbstatic
#undef dbglobal

#undef DB_Notice
#undef DB_Warning
#undef DB_Error

#undef DB_MAX_NAME
#undef DB_MAX_ENTRY
#undef DB_MAX_TABLES
#undef DB_MAX_QUERY
#undef DB_MAX_ALIVETIME

#undef DEFAULT_DATABASE_NAME
