#include "mongoBenchmarkImpl.cpp"
int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		nlogerr("need param: ./mongoBenchmark threadNum dbconnection tablename");
		return 0;
	}
	int threadNum = atoi(argv[1]);
	if (threadNum < 1)
	{
		nlogerr("threadNum in [1,1000] then thread = 1");
		threadNum = 1;
	}
	if (threadNum > 1000)
	{
		nlogerr("threadNum in [1,1000] then thread = 1");
		threadNum = 1000;
	}
	benchmark(threadNum, argv[2], argv[3]);
	return 0;
}