/*
    Easy Database - SQLITE manager (easydb.inc)
    * SQLITE has been the fastest and very powerful database language for samp!
	* This include allows you to write and run necessary queries without any SQL language
	* This is fast and has wide range of options and functions.
	* Also, this supports multiple SQL file handling at one time.

	Author: (creator)
 	* Gammix

	Contributors:
 	* Slice - Reading BUD's code really helped

	(c) Copyright 2015
 	* This file is provided as is (no warranties).
*/
/*
FUNCTIONS:
native DB::Open(name[]);
native DB::Close();
native DB::VerifyTable(tablename[], keyname[]);
native DB::VerifyColoumn(table, coloumn[], DB_COLOUMN_TYPES:type, {_,Float }:...);
native DB::RetrieveKey(table, coloumn[], {_,Float }:...);
native DB::CreateRow(table, coloumn[], {_,Float }:...);
native DB::DeleteRow(table, key);
native DB::IsValidRow(table, key);
native DB::GetIntEntry(table, key, coloumn[]);
native Float:DB::GetFloatEntry(table, key, coloumn[]);
native DB::GetStringEntry(table, key, coloumn[], str[], len = sizeof(str));
native DB::SetIntEntry(table, key, coloumn[], value);
native DB::SetFloatEntry(table, key, coloumn[], Float:value);
native DB::SetStringEntry(table, key, coloumn[], value[]);
native _
native DB::Hash(buffer[], len = sizeof(buffer), const str[]);
native DB::Escape(text[]);
native DB::CountTables();
native DB::CountColoumns(table);
*/

//

#define MAX_DATABASE_NAME           25 //max size of database file name

#define MAX_TABLES               	2 //max number of tables a database can have
#define MAX_TABLE_NAME           	25 //max size of table's name

#define MAX_COLOUMNS                20 //max coloumns a record can have
#define MAX_COLOUMN_NAME            50 //max coloumn name length

#define MAX_VALUE_SIZE            	50 //max value size (actually this is just used in setting default values)

//

#define INVALID_TABLE               -1
#define INVALID_KEY                 -1

//

#define DB:: 						DB_
#define private                   	stock static

//

enum DB_COLOUMN_TYPES
{
	DB::TYPE_NUMBER,
	DB::TYPE_FLOAT,
	DB::TYPE_STRING,
	DB::TYPE_INVALID
};

//

enum DB_TABLE_COLOUMN_ENUM
{
	e_COLOUMN_NAME[MAX_COLOUMN_NAME],
	e_COLOUMN_TYPE,
	bool:e_COLOUMN_EXIST
};
private DB_Table_Colorumn[MAX_TABLES][MAX_COLOUMNS][DB_TABLE_COLOUMN_ENUM];

//

enum DB_TABLE_ENUM
{
	e_TABLE_NAME[MAX_TABLE_NAME],
	e_TABLE_KEY_NAME[MAX_TABLE_NAME],
	bool:e_TABLE_EXIST
};
private DB_Table_General[MAX_TABLES][DB_TABLE_ENUM];

//

enum DB_MAIN_ENUM
{
	DB:e_DB_ID,
	bool:e_DB_OPENDED
};
private DB_Main[DB_MAIN_ENUM];

//

enum DB_INDEX_ENUM
{
	e_TABLE_COUNT,
	e_COLOUMN_COUNT[MAX_TABLES]
};
private DB_Index[DB_INDEX_ENUM];

//

//internal functions
private DB::IsNull(str[])
{
    if(	str[0] == 0 ||
		(str[0] == 1 && str[1] == 0))
	{
		return true;
    }
	return false;
}

private DB: DB::Get()
{
	return DB_Main[e_DB_ID];
}

private DB::IsValid()
{
	return DB_Main[e_DB_OPENDED];
}

private DB::IsValidTable(table)
{
	return DB_Table_General[table][e_TABLE_EXIST];
}

private DB_COLOUMN_TYPES:DB::GetColoumnType(table, coloumn[])
{
    for(new i = 0; i < MAX_COLOUMNS; i++)
	{
	    if(DB_Table_Colorumn[table][i][e_COLOUMN_EXIST])
	    {
			if(! strcmp(DB_Table_Colorumn[table][i][e_COLOUMN_NAME], coloumn))
			{
			    return DB_COLOUMN_TYPES:DB_Table_Colorumn[table][i][e_COLOUMN_TYPE];
			}
		}
	}
	return DB::TYPE_INVALID;
}

//main include functions
stock DB::Open(name[])
{
	//no null strings
	if(DB::IsNull(name))
	{
	    return false;
	}

	//check if database file is opened
	if(DB::IsValid())
	{
		DB::Close();
	}
	
	new DB:DB_File = db_open(name);
	if(DB_File)
	{
     	for(new i; i < MAX_TABLES; i++)
	    {
         	DB_Table_General[i][e_TABLE_EXIST] = false;
		}
		DB_Main[e_DB_ID] = DB_File;
		DB_Main[e_DB_OPENDED] = true;
		
		printf("[easydb.inc] Successfully opened database ''%s''.", name);
		return true;
	}
	printf("[easydb.inc] The database wasn't loaded ''%s'', the include will not run queries if any table isn't loaded.", name);
	return false;
}

stock DB::Close()
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return false;
	}

	for(new i; i < MAX_TABLES; i++)
	{
		DB_Table_General[i][e_TABLE_EXIST] = false;
	}
	DB_Main[e_DB_ID] = DB:0;
	DB_Main[e_DB_OPENDED] = false;
	
	print("[easydb.inc] The database was closed.");
	return false;
}

stock DB::VerifyTable(tablename[], keyname[])
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return INVALID_TABLE;
	}

	//no null strings
	if(	DB::IsNull(tablename) ||
		DB::IsNull(keyname))
	{
	    return INVALID_TABLE;
	}

	DB_Index[e_TABLE_COUNT] += 1;
	if(DB_Index[e_TABLE_COUNT] >= MAX_TABLES)
	{
	    DB_Index[e_TABLE_COUNT] -= 1;
	    print("[easydb.inc] An error has occured, the max tables limit has reached, the script couldn't verify/create the table.");
	    print("[easydb.inc] Please increase the limit by changing the value in ''MAX_TABLES''.");
	    return INVALID_TABLE;
	}

	new DB_Query[350];
	format(DB_Query, sizeof(DB_Query), "CREATE TABLE IF NOT EXISTS `%s` (`%s` INTEGER PRIMARY KEY)", tablename, keyname);
	db_free_result(db_query(DB::Get(), DB_Query));

	format(DB_Table_General[DB_Index[e_TABLE_COUNT]][e_TABLE_NAME], MAX_TABLE_NAME, tablename);
 	format(DB_Table_General[DB_Index[e_TABLE_COUNT]][e_TABLE_KEY_NAME], MAX_TABLE_NAME, keyname);
	DB_Table_General[DB_Index[e_TABLE_COUNT]][e_TABLE_EXIST] = true;
	return DB_Index[e_TABLE_COUNT];
}

stock DB::VerifyColoumn(table, coloumn[], DB_COLOUMN_TYPES:type, {_,Float }:...)
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return false;
	}

	//no null strings
	if(DB::IsNull(coloumn))
	{
	    return false;
	}

	//table validity check
	if(! DB::IsValidTable(table))
	{
	    return false;
	}

	//pass only valid types
	if(type == DB::TYPE_INVALID)
	{
		return false;
	}

	new DB_Query[350] = "";

	DB_Index[e_COLOUMN_COUNT][table] += 1;
	if(DB_Index[e_TABLE_COUNT] >= MAX_COLOUMNS)
	{
	    DB_Index[e_COLOUMN_COUNT][table] -= 1;
	    printf("[easydb.inc] An error has occured, the max columns limit has reached, the script couldn't verify/create the column for table id ''%i''.", table);
	    print("[easydb.inc] Please increase the limit by changing the value in ''MAX_COLOUMNS''.");
	    return false;
	}

	switch(type)
	{
		case DB::TYPE_NUMBER:
		{
			new DB_Default_Value = 0;
			if(numargs() == 3)
			{
				DB_Default_Value = getarg(2);
			}
			format(DB_Query, sizeof(DB_Query), "ALTER TABLE `%s` ADD COLUMN `%s` INTEGER DEFAULT(%d)", DB_Table_General[table][e_TABLE_NAME], coloumn, DB_Default_Value);
		}
		case DB::TYPE_FLOAT:
		{
			new Float:DB_Default_Value = 0.0;
			if(numargs() == 3)
			{
				DB_Default_Value = Float:getarg(2);
			}
			format(DB_Query, sizeof(DB_Query), "ALTER TABLE `%s` ADD COLUMN `%s` REAL DEFAULT(%f)", DB_Table_General[table][e_TABLE_NAME], coloumn, DB_Default_Value);
		}
		case DB::TYPE_STRING:
		{
			new DB_Default_Value[MAX_VALUE_SIZE] = "";
			if(numargs() == 3)
			{
			    new length = numargs();
				new count = 0;
				while(count += 1)
				{
				    if(count < length)
				    {
						format(DB_Default_Value, sizeof(DB_Default_Value), "%s", getarg(2, count));
						if(! strlen(DB_Default_Value))
						{
							break;
						}
						format(DB_Default_Value, sizeof(DB_Default_Value), "%s%s", DB_Default_Value, getarg(2, count));
					}
				}
				DB::Escape(DB_Default_Value);
			}
			format(DB_Query, sizeof(DB_Query), "ALTER TABLE `%s` ADD COLUMN `%s` TEXT DEFAULT('%s')", DB_Table_General[table][e_TABLE_NAME], coloumn, DB_Default_Value);
		}
		default:
		{
			return false;
		}
	}

	db_free_result(db_query(DB::Get(), DB_Query));
	format(DB_Table_Colorumn[table][DB_Index[e_COLOUMN_COUNT][table]][e_COLOUMN_NAME], MAX_COLOUMN_NAME, coloumn);
	DB_Table_Colorumn[table][DB_Index[e_COLOUMN_COUNT][table]][e_COLOUMN_TYPE] = (_:type);
	DB_Table_Colorumn[table][DB_Index[e_COLOUMN_COUNT][table]][e_COLOUMN_EXIST] = true;
	return true;
}

stock DB::RetrieveKey(table, coloumn[], {_,Float }:...)
{
	new DB_key = INVALID_KEY;

	//check if database file is opened
	if(! DB::IsValid())
	{
		return DB_key;
	}

	//no null strings
	if(DB::IsNull(coloumn))
	{
	    return DB_key;
	}

	//table validity check
	if(! DB::IsValidTable(table))
	{
	    return DB_key;
	}

	new DB_COLOUMN_TYPES:DB_ColoumnType = DB::GetColoumnType(table, coloumn);
	new DB_Query[350];
	switch(DB_ColoumnType)
	{
	    case DB::TYPE_NUMBER:
	    {
	        new DB_Default_Value = 0;
			if(numargs() == 3)
			{
				DB_Default_Value = getarg(2);
			}
			else
			{
				return DB_key;
			}
			format(DB_Query, sizeof(DB_Query), "SELECT `%s` FROM `%s` WHERE `%s`=%d", DB_Table_General[table][e_TABLE_KEY_NAME], DB_Table_General[table][e_TABLE_NAME], coloumn, DB_Default_Value);
	    }
	    case DB::TYPE_FLOAT:
	    {
	        new Float:DB_Default_Value = 0.0;
			if(numargs() == 3)
			{
				DB_Default_Value = Float:getarg(2);
			}
			else
			{
				return DB_key;
			}
			format(DB_Query, sizeof(DB_Query), "SELECT `%s` FROM `%s` WHERE `%s`='%f'", DB_Table_General[table][e_TABLE_KEY_NAME], DB_Table_General[table][e_TABLE_NAME], coloumn, DB_Default_Value);
	    }
	    case DB::TYPE_STRING:
	    {
	        new DB_Default_Value[MAX_VALUE_SIZE] = "";
	        if(numargs() == 3)
			{
				new length = numargs();
				new count = 0;
				while(count += 1)
				{
				    if(count < length)
				    {
						format(DB_Default_Value, sizeof(DB_Default_Value), "%s", getarg(2, count));
						if(! strlen(DB_Default_Value))
						{
							break;
						}
						format(DB_Default_Value, sizeof(DB_Default_Value), "%s%s", DB_Default_Value, getarg(2, count));
					}
				}
				DB::Escape(DB_Default_Value);
			}
			else
			{
				return DB_key;
			}
			format(DB_Query, sizeof(DB_Query), "SELECT `%s` FROM `%s` WHERE `%s`='%s' COLLATE NOCASE", DB_Table_General[table][e_TABLE_KEY_NAME], DB_Table_General[table][e_TABLE_NAME], coloumn, DB_Default_Value);
	    }
	}

	new DBResult:DB_Result = DBResult:0;
	DB_Result = db_query(DB::Get(), DB_Query);

	if(DB_Result)
	{
		DB_Query = "";
		db_get_field_assoc(DB_Result, DB_Table_General[table][e_TABLE_KEY_NAME], DB_Query, sizeof(DB_Query));

		db_free_result(DB_Result);
		
		DB_key = strval(DB_Query);
	}
	return DB_key;
}

stock DB::CreateRow(table, coloumn[], {_,Float }:...)
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return INVALID_KEY;
	}

	//no null strings
	if(DB::IsNull(coloumn))
	{
	    return INVALID_KEY;
	}

	//table validity check
	if(! DB::IsValidTable(table))
	{
	    return INVALID_KEY;
	}
	
	new DB_COLOUMN_TYPES:DB_ColoumnType = DB::GetColoumnType(table, coloumn);
	new DB_Query[350];
	switch(DB_ColoumnType)
	{
	    case DB::TYPE_NUMBER:
	    {
	        new DB_Default_Value = 0;
			if(numargs() == 3)
			{
				DB_Default_Value = getarg(2);
			}
			format(DB_Query, sizeof(DB_Query), "INSERT INTO `%s`(`%s`) VALUES(%d)", DB_Table_General[table][e_TABLE_NAME], coloumn, DB_Default_Value);
	    }
	    case DB::TYPE_FLOAT:
	    {
	        new Float:DB_Default_Value = 0.0;
			if(numargs() == 3)
			{
				DB_Default_Value = Float:getarg(2);
			}
			format(DB_Query, sizeof(DB_Query), "INSERT INTO `%s`(`%s`) VALUES('%f')", DB_Table_General[table][e_TABLE_NAME], coloumn, DB_Default_Value);
	    }
	    case DB::TYPE_STRING:
	    {
	        new DB_Default_Value[MAX_VALUE_SIZE] = "";
	        if(numargs() == 3)
			{
				new length = numargs();
				new count = 0;
				while(count += 1)
				{
				    if(count < length)
				    {
						format(DB_Default_Value, sizeof(DB_Default_Value), "%s", getarg(2, count));
						if(! strlen(DB_Default_Value))
						{
							break;
						}
						format(DB_Default_Value, sizeof(DB_Default_Value), "%s%s", DB_Default_Value, getarg(2, count));
					}
				}
				DB::Escape(DB_Default_Value);
			}
			format(DB_Query, sizeof(DB_Query), "INSERT INTO `%s`(`%s`) VALUES(`%s`)", DB_Table_General[table][e_TABLE_NAME], coloumn, DB_Default_Value);
	    }
	}

	new DBResult:DB_Result = DBResult:0;
	DB_Result = db_query(DB::Get(), DB_Query);

	if(DB_Result)
	{
		db_get_field_assoc(DB_Result, DB_Table_General[table][e_TABLE_KEY_NAME], DB_Query, sizeof(DB_Query));
		
		db_free_result(DB_Result);

		if(strval(DB_Query) >= 0)
		{
		    return strval(DB_Query);
		}
 	}
	return INVALID_KEY;
}

stock DB::DeleteRow(table, key)
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return false;
	}

	//table validity check
	if(! DB::IsValidTable(table))
	{
	    return false;
	}
	
	if(! DB::IsValidRow(table, key))
	{
		return false;
	}

	new DB_Query[350] = "";
	format(DB_Query, sizeof(DB_Query), "DELETE FROM `%s` WHERE `%s` = %d", DB_Table_General[table][e_TABLE_NAME], DB_Table_General[table][e_TABLE_KEY_NAME], key);

	db_free_result(db_query(DB::Get(), DB_Query));

	if(! DB::IsValidRow(table, key))
	{
		return true;
	}
	return false;
}

stock DB::IsValidRow(table, key)
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return false;
	}

	//table validity check
	if(! DB::IsValidTable(table))
	{
	    return false;
	}

	new DB_Query[350] = "";
	format(DB_Query, sizeof(DB_Query), "SELECT `%s` FROM `%s` WHERE `%s` = %d", DB_Table_General[table][e_TABLE_KEY_NAME], DB_Table_General[table][e_TABLE_NAME], DB_Table_General[table][e_TABLE_KEY_NAME], key);

	new DBResult:DB_Result = DBResult:0;
	DB_Result = db_query(DB::Get(), DB_Query);

	new bool:DB_Registered = false;

	if(DB_Result)
	{
		if(db_num_rows(DB_Result) >= 1)
		{
			DB_Registered = true;
		}

		db_free_result(DB_Result);
	}
	return DB_Registered;
}

stock DB::GetIntEntry(table, key, coloumn[])
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return false;
	}

	//no null strings
	if(DB::IsNull(coloumn))
	{
	    return false;
	}

	//table validity check
	if(! DB::IsValidTable(table))
	{
	    return false;
	}

	//if the type is valid
	if(DB::GetColoumnType(table, coloumn) != DB::TYPE_NUMBER)
	{
	    return false;
	}

	new DB_Query[350] = "";
	format(DB_Query, sizeof(DB_Query), "SELECT `%s` FROM `%s` WHERE `%s`=%d", coloumn, DB_Table_General[table][e_TABLE_NAME], DB_Table_General[table][e_TABLE_KEY_NAME], key);

	new DBResult:DB_Result = DBResult:0;
	DB_Result = db_query(DB::Get(), DB_Query);

	if(DB_Result)
	{
		DB_Query = "";
		db_get_field_assoc(DB_Result, DB_Table_General[table][e_TABLE_KEY_NAME], DB_Query, sizeof(DB_Query));

		db_free_result(DB_Result);

		return strval(DB_Query);
	}
	return false;
}

stock Float:DB::GetFloatEntry(table, key, coloumn[])
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return 0.0;
	}

	//no null strings
	if(DB::IsNull(coloumn))
	{
	    return 0.0;
	}

	//table validity check
	if(! DB::IsValidTable(table))
	{
	    return 0.0;
	}

	//if the type is valid
	if(DB::GetColoumnType(table, coloumn) != DB::TYPE_FLOAT)
	{
	    return 0.0;
	}

	new DB_Query[350] = "";
	format(DB_Query, sizeof(DB_Query), "SELECT `%s` FROM `%s` WHERE `%s`=%d", coloumn, DB_Table_General[table][e_TABLE_NAME], DB_Table_General[table][e_TABLE_KEY_NAME], key);

	new DBResult:DB_Result = DBResult:0;
	DB_Result = db_query(DB::Get(), DB_Query);

	if(DB_Result)
	{
		DB_Query = "";
		db_get_field_assoc(DB_Result, DB_Table_General[table][e_TABLE_KEY_NAME], DB_Query, sizeof(DB_Query));

		db_free_result(DB_Result);

		return floatstr(DB_Query);
	}
	return 0.0;
}

stock DB::GetStringEntry(table, key, coloumn[], str[], len = sizeof(str))
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return false;
	}

	//no null strings
	if(DB::IsNull(coloumn))
	{
	    return false;
	}

	//table validity check
	if(! DB::IsValidTable(table))
	{
	    return false;
	}

	//if the type is valid
	if(DB::GetColoumnType(table, coloumn) != DB::TYPE_STRING)
	{
	    return false;
	}

	new DB_Query[350] = "";
	format(DB_Query, sizeof(DB_Query), "SELECT `%s` FROM `%s` WHERE `%s`=%d", coloumn, DB_Names[(_:db)][0], DB_Names[(_:db)][1], key);

	new DBResult:DB_Result = DBResult:0;
	DB_Result = db_query(DB::Get(), DB_Query);

	if(DB_Result)
	{
		DB_Query = "";
		db_get_field_assoc(DB_Result, DB_Table_General[table][e_TABLE_KEY_NAME], str, len);

		db_free_result(DB_Result);

		return true;
	}
	return false;
}

stock DB::SetIntEntry(table, key, coloumn[], value)
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return false;
	}

	//no null strings
	if(DB::IsNull(coloumn))
	{
	    return false;
	}

	//table validity check
	if(! DB::IsValidTable(table))
	{
	    return false;
	}
	
	//if the type is valid
	if(DB::GetColoumnType(table, coloumn) != DB::TYPE_NUMBER)
	{
	    return false;
	}

	new DB_Query[350] = "";
	format(DB_Query, sizeof(DB_Query), "UPDATE `%s` SET `%s`=%d WHERE `%s`=%d", DB_Table_General[table][e_TABLE_NAME], coloumn, value, DB_Table_General[table][e_TABLE_KEY_NAME], key);

 	new DBResult:DB_Result = DBResult:0;
	DB_Result = db_query(DB::Get(), DB_Query);

	if(DB_Result)
	{
		db_free_result(DB_Result);
		return true;
	}
	return false;
}

stock DB::SetFloatEntry(table, key, coloumn[], Float:value)
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return false;
	}

	//no null strings
	if(DB::IsNull(coloumn))
	{
	    return false;
	}

	//table validity check
	if(! DB::IsValidTable(table))
	{
	    return false;
	}

	//if the type is valid
	if(DB::GetColoumnType(table, coloumn) != DB::TYPE_FLOAT)
	{
	    return false;
	}

	new DB_Query[350] = "";
	format(DB_Query, sizeof(DB_Query), "UPDATE `%s` SET `%s`=%f WHERE `%s`=%d", DB_Table_General[table][e_TABLE_NAME], coloumn, value, DB_Table_General[table][e_TABLE_KEY_NAME], key);

	new DBResult:DB_Result = DBResult:0;
	DB_Result = db_query(DB::Get(), DB_Query);

	if(DB_Result)
	{
		db_free_result(DB_Result);
		return true;
	}
	return false;
}

stock DB::SetStringEntry(table, key, coloumn[], value[])
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return false;
	}

	//no null strings
	if(DB::IsNull(coloumn))
	{
	    return false;
	}

	//table validity check
	if(! DB::IsValidTable(table))
	{
	    return false;
	}

	//if the type is valid
	if(DB::GetColoumnType(table, coloumn) != DB::TYPE_STRING)
	{
	    return false;
	}

	new DB_Query[350] = "";
	format(DB_Query, sizeof(DB_Query), "UPDATE `%s` SET `%s`=`%s` WHERE `%s`=%d", DB_Table_General[table][e_TABLE_NAME], coloumn, value, DB_Table_General[table][e_TABLE_KEY_NAME], key);

	new DBResult:DB_Result = DBResult:0;
	DB_Result = db_query(DB::Get(), DB_Query);

	if(DB_Result)
	{
		db_free_result(DB_Result);
		return true;
	}
	return false;
}

#if defined WP_Hash

	native DB::Hash(buffer[], len = sizeof(buffer), const str[]) = WP_Hash;

#else

	private DB::chrfind(needle, haystack[], start = 0) // Y_Less
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

	stock DB::Hash(buffer[], len = sizeof(buffer), const str[]) // Y_Less's dad (JSC)
	{
		private charset = \"4YLi6pOX)Mudvbc_IFVB/8HZ\2r(fGjaN0oU9C1Wywnq*smKQRxJDhkAS|53EzglT7tPe";
		private cells = 69;

		new j = strlen(buffer);
		new s = j;
		new t = 0;
		new m;

		for(new i = 0; i < len || i < j; i++)
		{
			m = i % len;
			t = (i >= j) ? __charset[(7 * i) % cells] : buffer[i];
			s = (s + DB::chrfind(t, __charset) + 1) % cells;
			bufferhash[m] = __charset[(s + bufferhash[m]) % cells];
		}
		bufferhash[len] = '\0';
	}
#endif

stock DB::Escape(text[])
{
	new ret[80 * 2], ch, i, j;
	while((ch = text[i++]) && j < sizeof (ret))
	{
		if(ch == '\'')
		{
			if(j < sizeof (ret) - 2)
			{
				ret[j++] = '\'';
				ret[j++] = '\'';
			}
		}
		else if(j < sizeof (ret))
		{
			ret[j++] = ch;
		}
		else
		{
			j++;
		}
	}
	ret[sizeof (ret) - 1] = '\0';
	return ret;
}

stock DB::CountTables()
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return false;
	}
	
	return DB_Index[e_TABLE_COUNT];
}

stock DB::CountColoumns(table)
{
	//check if database file is opened
	if(! DB::IsValid())
	{
		return false;
	}

	//table validity check
	if(! DB::IsValidTable(table))
	{
	    return false;
	}
	
	return DB_Index[e_COLOUMN_COUNT][table];
}
