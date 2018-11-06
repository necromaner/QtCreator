#include "yixin.h"
#include <QApplication>
#include <QTextCodec>
#include <QTranslator>

int main(int argc, char *argv[])
{
      QApplication a(argc, argv);
//   设置字符编码格式“UTF-8”
      QTextCodec *codec = QTextCodec::codecForName("UTF-8");
      QTextCodec::setCodecForLocale(codec);
      QFont F("WenQuanYi Zen Hei Mono");
      a.setFont(F);
//    主显示线程：图形用户接口，U盘检测，时间刷新，
      QApplication::setStyle(QStyleFactory::create("fusion"));
      PctGuiThread *PctGuiWindowP = new PctGuiThread();
      PctGuiWindowP->start();

      return a.exec();     // 让QApplication对象循环，即可以响应事件
}
