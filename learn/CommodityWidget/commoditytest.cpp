#include "commoditytest.h"
#include "commodity.h"
#include "commoditywidget.h"
#include "ui_commoditywidget.h"
void CommodityTest::case1_gui_costing()
{
    CommodityWidget w;
    //模拟按键，在键盘上输入成本 5.0
    QTest::keyClicks(w.ui->line_costing, "5.0");
    QCOMPARE(w.costing(), 5.0);
}
void CommodityTest::case2_gui_price()
{
    CommodityWidget w;
    //模拟按键，在键盘上输入价格 7.2
    QTest::keyClicks(w.ui->line_price, "7.2");
    QCOMPARE(w.price(), 7.2);
}
void CommodityTest::case3_gui_profit()
{
    CommodityWidget w;
    //模拟按键，在键盘上输入成本5.0，价格7.2
    //最后比较利润是否为2.2
    QTest::keyClicks(w.ui->line_costing, "5.0");
    QTest::keyClicks(w.ui->line_price, "7.2");
    QCOMPARE(w.profit(), 2.2);
}
