#include <iostream>
#include "AboutPage.h"

AboutPage::AboutPage(QWidget * parent , Qt::WindowFlags flags )
	: ConfigPage(parent,flags)
{
    ui.setupUi(this);

    ui.widget->close_button->hide();
}

AboutPage::~AboutPage()
{
}

void AboutPage::load()
{
    std::cerr << "Loading AboutPage!" << std::endl;
}
