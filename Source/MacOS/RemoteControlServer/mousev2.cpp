#include "converter.h"
#include "mousev2.h"
#include "settings.h"

#include <math.h>

#ifdef Q_OS_MAC
    #include "keyboardmac.h"
    #include "mousemac.h"
#endif

#include <QDateTime>
#include <QDesktopWidget>

#include <QDebug>

MouseV2 *MouseV2::instance = NULL;

MouseV2 *MouseV2::Instance()
{
    if (!instance)
    {
        instance = new MouseV2();
    }
    return instance;
}

MouseV2::MouseV2()
{
    currentGesture = 0;

    cursorPositionNew = new QPoint();
    cursorPositionCurrent = new QPoint();
    cursorPositionDown = new QPoint();
    P_ORIGIN = new QPoint(0, 0);
    P1_New = new QPoint();
    P2_New = new QPoint();
    P3_New = new QPoint();
    P1_Rel = new QPoint();
    P2_Rel = new QPoint();
    P1_Start = new QPoint();
    P2_Start = new QPoint();
    P3_Start = new QPoint();
    P1_Last = new QPoint();
    P2_Last = new QPoint();
    P3_Last = new QPoint();

    P1_Down = QDateTime::currentDateTime().toMSecsSinceEpoch();
    P1_Up = QDateTime::currentDateTime().toMSecsSinceEpoch();
    P2_Down = QDateTime::currentDateTime().toMSecsSinceEpoch();
    P2_Up = QDateTime::currentDateTime().toMSecsSinceEpoch();
}

QPoint *MouseV2::getPointAt(QByteArray &messageBytes, int ID)
{
    return new QPoint();
    // todo
}

QPoint *MouseV2::commandGetPoint(QByteArray &messageBytes, int ID)
{
    QPoint *p = new QPoint();
    if (ID == 0 && messageBytes.length() >= 5)
    {
        unsigned char firstByte = messageBytes.at(3);
        unsigned char secondByte = messageBytes.at(4);
        p->setX(firstByte);
        p->setY(secondByte);
    }
    else if (ID == 1 && messageBytes.length() >= 7)
    {
        unsigned char firstByte = messageBytes.at(5);
        unsigned char secondByte = messageBytes.at(6);
        p->setX(firstByte);
        p->setY(secondByte);
    }
    return p;
}

bool MouseV2::isPointerDown(QPoint &point)
{
    return (point.x() == P_ORIGIN->x() && point.y() == P_ORIGIN->y()) ? false : true;
}

void MouseV2::moveCursor()
{
    cursorPositionCurrent = MouseMac::Instance()->getCursorPosition();
    if (isPointerDown(*P2_New))
    {
        // 2 Pointer down
        if (mouseLeftDown)
        {
            // Left mouse down and moving
            P2_Rel->setX((P2_New->x() - P2_Last->x()) * Settings::Instance()->mouseSensitivity);
            P2_Rel->setY((P2_New->y() - P2_Last->y()) * Settings::Instance()->mouseSensitivity);
            cursorPositionNew = new QPoint(cursorPositionCurrent->x() + P2_Rel->x(), cursorPositionCurrent->y() + P2_Rel->y());
            P2_Last = new QPoint(P2_New->x(), P2_New->y());
            MouseMac::Instance()->moveMouseTo(cursorPositionNew->x(), cursorPositionNew->y());
        }
    }
    else
    {
        // 1 Pointer down
        if (P1_Last->x() == P_ORIGIN->x() && P1_Last->y() == P_ORIGIN->y())
        {
            // Start of mouse move
            P1_Last = new QPoint(P1_New->x(), P1_New->y());
        }
        else
        {
            if (Settings::Instance()->mouseAcceleration == 1)
            {
                // No acceleration
                P1_Rel->setX((P1_New->x() - P1_Last->x()) * Settings::Instance()->mouseSensitivity);
                P1_Rel->setY((P1_New->y() - P1_Last->y()) * Settings::Instance()->mouseSensitivity);
            }
            else
            {
                // Acceleration
                P1_Rel->setX((P1_New->x() - P1_Last->x()));
                P1_Rel->setY((P1_New->y() - P1_Last->y()));

                if (P1_Rel->x() > 0)
                {
                    P1_Rel->setX(pow(P1_Rel->x(), Settings::Instance()->mouseAcceleration) * Settings::Instance()->mouseSensitivity);
                }
                else
                {
                    P1_Rel->setX(P1_Rel->x() * -1);
                    P1_Rel->setX(pow(P1_Rel->x(), Settings::Instance()->mouseAcceleration) * Settings::Instance()->mouseSensitivity);
                    P1_Rel->setX(P1_Rel->x() * -1);
                }

                if (P1_Rel->y() > 0)
                {
                    P1_Rel->setY(pow(P1_Rel->y(), Settings::Instance()->mouseAcceleration) * Settings::Instance()->mouseSensitivity);
                }
                else
                {
                    P1_Rel->setY(P1_Rel->y() * -1);
                    P1_Rel->setY(pow(P1_Rel->y(), Settings::Instance()->mouseAcceleration) * Settings::Instance()->mouseSensitivity);
                    P1_Rel->setY(P1_Rel->y() * -1);
                }

                cursorPositionNew = new QPoint(cursorPositionCurrent->x() + P1_Rel->x(), cursorPositionCurrent->y() + P1_Rel->y());
                P1_Last = new QPoint(P1_New->x(), P1_New->y());
                MouseMac::Instance()->moveMouseTo(cursorPositionNew->x(), cursorPositionNew->y());
            }
        }
    }
}

void MouseV2::leftClickRepeat(int count)
{

}

void MouseV2::zoom(int value, int count)
{
    for (int i = 0; i < count; ++i)
    {
#ifdef Q_OS_MAC
        int keycode = (value == 1) ? KeyboardMac::KEYCODE_ZOOM_IN : KeyboardMac::KEYCODE_ZOOM_OUT;
        KeyboardMac::Instance()->sendShortcut(keycode);
#endif
    }
}

void MouseV2::processMultitouch()
{
    P3_New->setX(round((P1_New->x() + P2_New->x()) / 2.0));
    P3_New->setY(round((P1_New->y() + P2_New->y()) / 2.0));
    // todo
}

void MouseV2::parseCursorMove(QByteArray &messageBytes)
{
    P1_New = commandGetPoint(messageBytes, 0);
    if (isMultitouch)
    {
        P2_New = commandGetPoint(messageBytes, 1);
        processMultitouch();
    }
    else
    {
        P2_New = new QPoint(P_ORIGIN->x(), P_ORIGIN->y());
        moveCursor();
    }
}

void MouseV2::parseCursorSet(QByteArray &messageBytes)
{
    int x = 0;
    int y = 0;
    if (messageBytes.length() >= 7)
    {
        unsigned char firstByte = messageBytes.at(3);
        unsigned char secondByte = messageBytes.at(4);
        x = firstByte << 8 | secondByte;
        firstByte = messageBytes.at(5);
        secondByte = messageBytes.at(6);
        y = firstByte << 8 | secondByte;
#ifdef Q_OS_MAC
        MouseMac::Instance()->moveMouseTo(x, y);
#endif
    }
}

void MouseV2::parseScroll(QByteArray &messageBytes)
{
    P1_New = commandGetPoint(messageBytes, 0);
    scrollAmount = (P1_New->y() - P1_Last->y()) * 20;
#ifdef Q_OS_MAC
    if (P1_New->y() > P1_Last->y()) MouseMac::Instance()->mouseScrollVertical(-1, scrollAmount);
    else if (P1_New->y() < P1_Last->y()) MouseMac::Instance()->mouseScrollVertical(1, scrollAmount);
#endif
    P1_Last = new QPoint(P1_New->x(), P1_New->y());
}

void MouseV2::parseClick(QByteArray &messageBytes)
{
    if (messageBytes.length() >= 3)
    {
        switch (messageBytes.at(2))
        {
        case cmd_mouse_down:
            isMultitouch = false;
            P1_Start = commandGetPoint(messageBytes, 0);
            P1_Last = new QPoint(P1_Start->x(), P1_Start->y());
#ifdef Q_OS_MAC
            cursorPositionDown = MouseMac::Instance()->getCursorPosition();
#endif
            mousePadDown = true;
            P1_Down = QDateTime::currentDateTime().toMSecsSinceEpoch();
            break;
        case cmd_mouse_up:
            mousePadDown = false;
            isMultitouch = false;

            P1_Last = new QPoint(P_ORIGIN->x(), P_ORIGIN->y());
            P2_Last = new QPoint(P_ORIGIN->x(), P_ORIGIN->y());
            P1_New = new QPoint(P_ORIGIN->x(), P_ORIGIN->y());
            P1_Up = QDateTime::currentDateTime().toMSecsSinceEpoch();

            if (P1_Up - P1_Down > 150
                    && -10 < P1_Start->x() - P1_Last->x() < 10 && -10 < P1_Start->y() - P1_Last->y() < 10)
            {
                leftClickRepeat(1);
            }
            break;
        case cmd_mouse_down_1:
            isMultitouch = true;
            P1_Start = commandGetPoint(messageBytes, 0);
            P1_New = new QPoint(P1_Start->x(), P1_Start->y());
            P1_Last = new QPoint(P1_Start->x(), P1_Start->y());
            P1_Down = QDateTime::currentDateTime().toMSecsSinceEpoch();

            P2_Start = commandGetPoint(messageBytes, 1);
            P2_New = new QPoint(P2_Start->x(), P2_Start->y());
            P2_Last = new QPoint(P2_Start->x(), P2_Start->y());
            P2_Down = QDateTime::currentDateTime().toMSecsSinceEpoch();
            currentGesture = GESTURE_NONE;

            P3_Start->setX(round((P1_New->x() + P2_New->x()) / 2.0));
            P3_Start->setY(round((P1_New->y() + P2_New->y()) / 2.0));
            P3_New = new QPoint(P3_Start->x(), P3_Start->y());
            P3_Last = new QPoint(P3_Start->x(), P3_Start->y());
            P3_Vector_Start = Converter::Instance()->getPointDistance(*P1_New, *P2_New);
            break;
        case cmd_mouse_up_1:
            P2_Up = QDateTime::currentDateTime().toMSecsSinceEpoch();
            isMultitouch = false;
            P1_Last = new QPoint(P_ORIGIN->x(), P_ORIGIN->y());
            P2_New = new QPoint(P_ORIGIN->x(), P_ORIGIN->y());
            P3_New = new QPoint(P_ORIGIN->x(), P_ORIGIN->y());
            P3_Last = new QPoint(P_ORIGIN->x(), P_ORIGIN->y());
            P3_Start = new QPoint(P_ORIGIN->x(), P_ORIGIN->y());
            break;
        case cmd_mouse_left_down:
#ifdef Q_OS_MAC
            MouseMac::Instance()->leftMouseDown(false);
#endif
            dateLeftDown = QDateTime::currentDateTime().toMSecsSinceEpoch();
            mouseLeftDown = true;
            break;
        case cmd_mouse_left_up:
#ifdef Q_OS_MAC
            MouseMac::Instance()->leftMouseUp(false);
#endif
            dateLeftUp = QDateTime::currentDateTime().toMSecsSinceEpoch();
            mouseLeftDown = false;
            break;
        case cmd_mouse_right_down:
#ifdef Q_OS_MAC
            MouseMac::Instance()->rightMouseDown();
#endif
            mouseRightDown = true;
            break;
        case cmd_mouse_right_up:
#ifdef Q_OS_MAC
            MouseMac::Instance()->rightMouseUp();
#endif
            mouseRightDown = false;
            break;
        case cmd_mouse_left_click:
#ifdef Q_OS_MAC
            MouseMac::Instance()->leftMouseDown(false);
            MouseMac::Instance()->leftMouseUp(false);
#endif
            break;
        case cmd_mouse_right_click:
#ifdef Q_OS_MAC
            MouseMac::Instance()->rightMouseDown();
            MouseMac::Instance()->rightMouseUp();
#endif
            break;
        }
    }
}

void MouseV2::parsePointer(QByteArray &messageBytes)
{
    if (messageBytes.length() >= 5)
    {
        unsigned char firstByte = messageBytes.at(3);
        unsigned char secondByte = messageBytes.at(4);
        X_New = firstByte;
        Y_New = secondByte;

        if (X_New > 99) X_New = 100 - X_New;
        if (Y_New > 99) Y_New = 100 - Y_New;

        X_New = X_New - X_Def;
        Y_New = Y_New - Y_Def;

        X_Rel = ((Y_New * -1) * Settings::Instance()->motionAcceleration * 20) / 100;
        Y_Rel = ((X_New * 1) * Settings::Instance()->motionAcceleration * 20) / 100;

        if (!(X_Rel > Settings::Instance()->motionFilter || X_Rel < Settings::Instance()->motionFilter * -1))
        {
            X_Rel = 0;
        }
        if (!(Y_Rel > Settings::Instance()->motionFilter || Y_Rel < Settings::Instance()->motionFilter * -1))
        {
            Y_Rel = 0;
        }

        // Pointer
        if (!(X_Rel == 0 && Y_Rel == 0))
        {
//            cursorPositonCurrent = Server.pointer.getPointerPosition()
            if (Y_Rel < 0) cursorPositionNew->setX(cursorPositionCurrent->x() - Y_Rel * Y_Rel);
            else if (Y_Rel > 0) cursorPositionNew->setX(cursorPositionCurrent->x() + Y_Rel * Y_Rel);
            else cursorPositionNew->setX(cursorPositionCurrent->x());

            if (X_Rel < 0) cursorPositionNew->setY(cursorPositionCurrent->y() - X_Rel * X_Rel);
            else if (X_Rel > 0) cursorPositionNew->setY(cursorPositionCurrent->y() + X_Rel * X_Rel);
            else cursorPositionNew->setY(cursorPositionCurrent->y());

//            if (cursorPositionNew->x() > Screen.PrimaryScreen.Bounds.Width - 50 Then
//                        cursorPositonNew.X = Screen.PrimaryScreen.Bounds.Width - 50
//                    ElseIf cursorPositonNew.X < 0 Then
//                        cursorPositonNew.X = 0
//                    End If
//                    If cursorPositonNew.Y > Screen.PrimaryScreen.Bounds.Height - 50 Then
//                        cursorPositonNew.Y = Screen.PrimaryScreen.Bounds.Height - 50
//                    ElseIf cursorPositonNew.Y < 0 Then
//                        cursorPositonNew.Y = 0
//                    End If
            //todo

//                    Server.pointer.setPointerPosition(New Point(cursorPositonNew.X, cursorPositonNew.Y))
        }
    }
}

void MouseV2::calibratePointer(QByteArray &messageBytes)
{
    if (messageBytes.length() >= 5)
    {
        unsigned char firstByte = messageBytes.at(3);
        unsigned char secondByte = messageBytes.at(4);
        X_Def = firstByte;
        Y_Def = secondByte;

        if (X_Def > 99) X_Def = 100 - X_Def;
        if (Y_Def > 99) Y_Def = 100 - Y_Def;
    }
    else
    {
        X_Def = 0;
        Y_Def = 0;
    }
}

void MouseV2::parseLaser(QByteArray &messageBytes)
{
    QPoint *point_org, *point;
    point_org = getPointAt(messageBytes, 0);

    // Server.pointer.showPointer()

    QDesktopWidget desktop;
    QRect mainScreenSize = desktop.screenGeometry(desktop.primaryScreen());

    int width = mainScreenSize.width();
    int height = mainScreenSize.height();

    point->setX(point_org->x() * width / 255);
    point->setY(point_org->y() * height / 255);

    // todo
    // Server.pointer.setPointerPosition(...)
    // Server.pointer.fadeOutPointer()
}

void MouseV2::parseMouse(QString cmd)
{
    qDebug() << "parseMouse";
}