#include "ui/TitleBar.h"
#include "util/AppIcon.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QWindow>

TitleBar::TitleBar(QWidget *parent) : QWidget(parent) {
    auto *iconLabel = new QLabel(this);
    iconLabel->setPixmap(AppIcon::build().pixmap(20, 20));
    iconLabel->setFixedSize(20, 20);
    // So a press/drag starting on the icon still moves the window via
    // TitleBar's own mouse handling, instead of the plain QLabel eating it.
    iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    titleLabel_ = new QLabel(QStringLiteral("csjplayer"), this);
    minimizeButton_ = new QPushButton(QStringLiteral("─"), this); // ─
    maximizeButton_ = new QPushButton(QStringLiteral("□"), this); // □
    closeButton_ = new QPushButton(QStringLiteral("✕"), this);    // ✕

    for (QPushButton *button : {minimizeButton_, maximizeButton_, closeButton_}) {
        button->setFixedSize(36, 28);
        button->setObjectName(QStringLiteral("titleBarButton"));
    }
    closeButton_->setObjectName(QStringLiteral("titleBarCloseButton"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(iconLabel);
    layout->addSpacing(8);
    layout->addWidget(titleLabel_, /*stretch=*/1);
    layout->addWidget(minimizeButton_);
    layout->addWidget(maximizeButton_);
    layout->addWidget(closeButton_);

    connect(minimizeButton_, &QPushButton::clicked, this, &TitleBar::minimizeClicked);
    connect(maximizeButton_, &QPushButton::clicked, this, &TitleBar::maximizeRestoreClicked);
    connect(closeButton_, &QPushButton::clicked, this, &TitleBar::closeClicked);
}

void TitleBar::setTitleText(const QString &text) {
    titleLabel_->setText(text);
}

void TitleBar::setMaximized(bool maximized) {
    maximizeButton_->setText(maximized ? QStringLiteral("▣") : QStringLiteral("□"));
}

void TitleBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        pendingDrag_ = true;
        dragStartGlobalPos_ = event->globalPosition().toPoint();
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent *event) {
    if (pendingDrag_ && (event->buttons() & Qt::LeftButton)) {
        const QPoint delta = event->globalPosition().toPoint() - dragStartGlobalPos_;
        if (delta.manhattanLength() >= QApplication::startDragDistance()) {
            pendingDrag_ = false;
            if (QWindow *handle = window()->windowHandle()) {
                handle->startSystemMove();
            }
            event->accept();
            return;
        }
    }
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        pendingDrag_ = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit maximizeRestoreClicked();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}
