#include <capi.h>

using namespace std;
using namespace interop;

int main(int argc, char *argv[])
{
  vector<const char*> include = { "C:\\Programs\\CSVUtils\\inc\\" };
  string path                 = "C:\\Programs\\CSVUtils\\obj\\csv-test.i";

  {
    CAPI test = CAPI(path.c_str(), include);

    test.makeC(""); // random test
  }

  return 0;
}