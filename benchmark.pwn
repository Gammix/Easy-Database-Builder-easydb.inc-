#include <a_samp>
#include <easydb>

main()
{
	new t = GetTickCount();
	DB::Open("test.db");
	printf("Time taken to open: %i", GetTickCount() - t);

	t = GetTickCount();
	DB::VerifyTable("test", "key");
	printf("Time taken to verify table: %i", GetTickCount() - t);

	t = GetTickCount();
	DB::VerifyColumn(0, "int", DB::TYPE_NUMBER, 0);
	printf("Time taken to verify int column: %i", GetTickCount() - t);

	t = GetTickCount();
	DB::VerifyColumn(0, "float", DB::TYPE_FLOAT, 0.0);
	printf("Time taken to verify float column: %i", GetTickCount() - t);

	t = GetTickCount();
	DB::VerifyColumn(0, "string", DB::TYPE_STRING, "");
	printf("Time taken to verify string column: %i", GetTickCount() - t);

	t = GetTickCount();
	DB::CreateRow(0, "int", 1);
	printf("Time taken to create row: %i", GetTickCount() - t);

	t = GetTickCount();
	new i = DB::RetrieveKey(0, "int", 1);
	printf("Time taken to get row/key id: %i (key: %i)", GetTickCount() - t, i);

	t = GetTickCount();
	DB::SetIntEntry(0, i, "int", 1);
	printf("Time taken to set int: %i", GetTickCount() - t);

	t = GetTickCount();
	DB::SetFloatEntry(0, i, "float", 1.0);
	printf("Time taken to set float: %i", GetTickCount() - t);

	t = GetTickCount();
	DB::SetStringEntry(0, i, "string", "str");
	printf("Time taken to set string: %i", GetTickCount() - t);

	t = GetTickCount();
	new intval = DB::GetIntEntry(0, i, "int");
	printf("Time taken to get int: %i (value: %i)", GetTickCount() - t, intval);

	t = GetTickCount();
	new Float:floatval = DB::GetFloatEntry(0, i, "float");
	printf("Time taken to get float: %i (value: %f)", GetTickCount() - t, floatval);

	t = GetTickCount();
	new stringval[25];
	DB::GetStringEntry(0, i, "string", stringval);
	printf("Time taken to get string: %i (value: %s)", GetTickCount() - t, stringval);
}
