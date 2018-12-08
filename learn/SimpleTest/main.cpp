#include <iostream>
#include "commodity.h"
#include "commoditytest.h"
using std::cout;


int cntPass = 0;
int cntFail = 0;
void test(bool b)
{
    if (b)
        ++cntPass;
    else
        ++cntFail;
}

QTEST_MAIN(CommodityTest);
//int main(int argc, char *argv[])
//{
//    //啤酒，成本4块卖5块
//    Commodity c("beer_1", "啤酒", 4, 5);
//    test(c.serialNum() == "beer_1");
//    test(c.name()      == "啤酒");
//    test(c.consting()  == 4);
//    test(c.price()     == 5);
//    test(c.profit()    == 1);
//    cout << "cntPass:" << cntPass << "\ncntFail:" << cntFail;
//}
