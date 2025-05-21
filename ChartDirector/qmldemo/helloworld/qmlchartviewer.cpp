#include "qmlchartviewer.h"
#include "chartdir.h"
#include <QFile>
#include <QQuickWindow>


/////////////////////////////////////////////////////////////////////////////
//
// QChartImage
//
/////////////////////////////////////////////////////////////////////////////

//
// Constants used in m_delayChartUpdate
//
enum { NO_DELAY, NEED_DELAY, NEED_UPDATE };


QmlChartImage::QmlChartImage(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    // High dpi support
    m_dpi = 0;
    m_toImageScaleX = 1;
    m_toImageScaleY = 1;
}

QmlChartImage::~QmlChartImage()
{
}

void QmlChartImage::paint(QPainter *painter)
{
    painter->drawPixmap(QRect(0, 0, width(), height()), m_chartImage);

    if ((m_lineColor.alpha() != Chart::Transparent) && (m_lineWidth > 0))
    {
        painter->setPen(QPen(m_lineColor, m_lineWidth));
        painter->drawRect(m_rect);
    }

    if (!m_toolTipImage.isNull())
    {
        QPoint p = QCursor::pos();
        p = mapFromGlobal(QPointF(p.x(), p.y())).toPoint();

        int x = p.x();
        int y = p.y() + 20;

        if (x + m_toolTipImage.width() > width())
        {
            x = width() - m_toolTipImage.width();
            if (x < 0)
                x = 0;
        }
        if (y + m_toolTipImage.height() > height())
        {
            y = p.y() - m_toolTipImage.height() - 2;
            if (y < 0)
                y = 0;
        }

        painter->drawPixmap(QPoint(x, y), m_toolTipImage);
    }
}

void QmlChartImage::displayChart(BaseChart *c)
{
    // Render chart into QPixmap
    m_chartImage = makePixmap(c);
    setSize(QSizeF((int)(m_chartImage.width() / m_chartImage.devicePixelRatioF() + 0.5),
                   (int)(m_chartImage.height() / m_chartImage.devicePixelRatioF() + 0.5)));

    update();

    // High dpi support
    m_toImageScaleX = ((width() <= 0) || !c) ? 1 : (c->getDrawArea()->getWidth() / (double)width());
    m_toImageScaleY = ((height() <= 0) || !c) ? 1 : (c->getDrawArea()->getHeight() / (double)height());
}

QPixmap QmlChartImage::makePixmap(BaseChart *c)
{
    QPixmap image;
    if (0 != c)
    {
        int dpi = getDpi();
        if (dpi > 0)
        {
            char buffer[1024];
            sprintf(buffer, "+dpi=%d", dpi);
            c->setOutputOptions(buffer);
        }
        MemBlock m = c->makeChart(Chart::QTIMG);
        image.loadFromData((uchar *)m.data, (uint)m.len);
        double pixelRatio = (dpi > 0) ? dpi/96.0 : 1;
        image.setDevicePixelRatio(pixelRatio);
    }
    return image;
}

void QmlChartImage::setDpi(int dpi)
{
    m_dpi = dpi;
}

int QmlChartImage::getDpi()
{
    int dpi = m_dpi;
    if (dpi < 0)
        return dpi;
    if (dpi == 0)
    {
        QQuickWindow *w = window();
        double windowDpi = (w ? w->effectiveDevicePixelRatio() : 1);
        dpi = (int)(96 * windowDpi + 0.5);
    }
    return (dpi < 24) ? 24 : ((dpi > 384) ? 384 : dpi);
}

double QmlChartImage::toImageX(int x)
{
    return x * m_toImageScaleX;
}
double QmlChartImage::toImageY(int y)
{
    return y * m_toImageScaleY;
}

int QmlChartImage::toDisplayX(double x)
{
    double ret = x / m_toImageScaleX;
    return (int)((ret >= 0) ? ret + 0.5 : (ret - 0.5));
}
int QmlChartImage::toDisplayY(double y)
{
    double ret = y / m_toImageScaleY;
    return (int)((ret >= 0) ? ret + 0.5 : (ret - 0.5));
}

void QmlChartImage::setRect(int x, int y, int w, int h, QColor lineColor, int lineWidth)
{
    if (((lineColor.alpha() == 0) || (lineWidth <= 0)) &&
       ((m_lineColor.alpha() == 0) || (m_lineWidth <= 0)))
        return;

    if ((x == m_rect.left()) && (y == m_rect.top()) && (w == m_rect.width()) &&
        (h == m_rect.height()) && (lineColor == m_lineColor) && (lineWidth == m_lineWidth))
        return;

    m_lineColor = lineColor;
    m_lineWidth = lineWidth;
    m_rect = QRect(x, y, w, h);
    update();
}

void QmlChartImage::setToolTip(QPoint pos, QString text)
{
    if (text.isEmpty() && m_toolTip.isEmpty())
        return;

    if ((pos == m_toolTipPos) && (text == m_toolTip))
        return;

    m_toolTipPos = pos;

    if (m_toolTip != text)
    {
        m_toolTip = text;
        if (m_toolTip.isEmpty())
            m_toolTipImage = QPixmap();
        else
        {
            PieChart c(1, 1, Chart::Transparent);
            c.makeChart()->renderCDML(text.toUtf8().data());
            m_toolTipImage = makePixmap(&c);
        }
    }
    update();
}

void QmlChartImage::moveToolTip(QPoint pos)
{
    if ((pos != m_toolTipPos) && !m_toolTipImage.isNull())
    {
        pos = m_toolTipPos;
        update();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
// Mouse cursors for zooming and scrolling support
//
/////////////////////////////////////////////////////////////////////////////

static class QASE_CursorManager
{
private:
    QCursor *hZoomInCursor;
    QCursor *hZoomOutCursor;
    QCursor *hNoZoomCursor;

    QCursor &getZoomCursor(QCursor **cache, double scale, int flags)
    {
        if (0 == *cache)
        {
            DrawArea d;
            d.setSize(32, 32, Chart::Transparent);
            d.circle(15, 15, 7, 7, 0x000000, 0xffffff);
            if (flags & 0x1)
                d.hline(12, 18, 15, 0x000000);
            if (flags & 0x2)
                d.vline(12, 18, 15, 0x000000);
            int x[] = { 21, 28, 26, 19 };
            int y[] = { 19, 26, 28, 21 };
            d.polygon(IntArray(x, 4), IntArray(y, 4), 0x000000, 0x000000);
            char buffer[1024];
            sprintf(buffer, "dpi=%d", (int)(96 * scale + 0.5));
            d.setOutputOptions(buffer);
            MemBlock m = d.outPNG();
            QImage img = QImage::fromData((const uchar*)m.data, m.len);
            img.setDevicePixelRatio(scale);
            *cache = new QCursor(QPixmap::fromImage(img), 15, 15);
        }
        return **cache;
    }

public:
    QASE_CursorManager() :
        hZoomInCursor(0), hZoomOutCursor(0), hNoZoomCursor(0)
    {
    }
    ~QASE_CursorManager()
    {
        delete hZoomInCursor;
        delete hZoomOutCursor;
        delete hNoZoomCursor;
    }
    QCursor &getZoomInCursor(double scale)
    {
        return getZoomCursor(&hZoomInCursor, scale, 3);
    }
    QCursor &getZoomOutCursor(double scale)
    {
        return getZoomCursor(&hZoomOutCursor, scale, 1);
    }
    QCursor &getNoZoomCursor(double scale)
    {
        return getZoomCursor(&hNoZoomCursor, scale, 0);
    }
} cursorManager;


QmlChartViewer::QmlChartViewer(QQuickItem *parent) : QmlChartImage(parent)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setAntialiasing(true);

    // High dpi support
    m_currentChart = 0;
    m_hotSpotTester = 0;
    m_imageMapChart = 0;

    // initialize chart configuration
    m_selectBoxLineColor = QColor(0, 0, 0);
    m_selectBoxLineWidth = 2;
    m_mouseUsage = MouseUsageDefault;
    m_zoomDirection = Chart::DirectionHorizontal;
    m_zoomInRatio = 2;
    m_zoomOutRatio = 0.5;
    m_mouseWheelZoomRatio = 1;
    m_scrollDirection = Chart::DirectionHorizontal;
    m_minDragAmount = 5;
    m_updateInterval = 20;

    // current state of the mouse
    m_isOnPlotArea = false;
    m_isPlotAreaMouseDown = false;
    m_isDragScrolling = false;
    m_currentHotSpot = -1;
    m_isClickable = false;
    m_isMouseTracking = false;
    m_isInMouseMove = false;

    // chart update rate support
    m_needUpdateChart = false;
    m_needUpdateImageMap = false;
    m_holdTimerActive = false;
    m_isInViewPortChanged = false;
    m_delayUpdateChart = NO_DELAY;
    m_delayedChart = 0;
    m_lastMouseMove = 0;
    m_delayedMouseEvent = 0;
    m_delayImageMapUpdate = false;

    // track cursor support
    m_autoHideMsg = "";
    m_currentMouseX = -0x3fffffff;
    m_currentMouseY = -0x3fffffff;
    m_isInMouseMovePlotArea = false;

    // tooltip
    m_CDMLToolTipPrefix = "<*block,bgColor=F0F0F0,edgeColor=808080,margin=5,roundedCorners=3*><*font,color=222222*>";
}

QmlChartViewer::~QmlChartViewer()
{
    delete m_hotSpotTester;
    delete m_delayedMouseEvent;
    delete m_delayedChart;
    delete m_imageMapChart;
}

namespace {
    static class QtResourceLoader {
    public:
        QtResourceLoader() { Chart::setResourceLoader(loader); }
        static bool loader(const char *id, char *(allocator)(int), char **data, int *len) {
            QFile f(QString(":/") + id);
            return f.open(QIODevice::ReadOnly) && (0 < (*len = f.size())) &&
                (0 != (*data = allocator(*len))) && f.read(*data, *len);
        }
    } QtResourceLoaderRegistration;
}

//
// Update the display
//
void QmlChartViewer::updateDisplay()
{
    if (m_delayUpdateChart == NO_DELAY)
    {
        commitUpdateChart();
    }
    else
    {
        m_delayUpdateChart = NEED_UPDATE;
        delete m_delayedChart;
        m_delayedChart = (0 != m_currentChart) ? new BaseChart(m_currentChart) : 0;
    }
}

//
// Commit chart to display
//
void QmlChartViewer::commitUpdateChart()
{
    if (m_delayUpdateChart == NEED_DELAY)
    {
        // No actual update occur
        m_delayUpdateChart = NO_DELAY;
        return;
    }

    // Display the chart
    BaseChart *c = (m_delayUpdateChart == NEED_UPDATE) ? m_delayedChart : m_currentChart;
    displayChart(c);

    // Set the chart metrics and clear the image map
    setChartMetrics(c ? c->getChartMetrics() : 0);

    // Any delayed chart has been committed
    m_delayUpdateChart = NO_DELAY;
    delete m_delayedChart;
    m_delayedChart = 0;
}

//
// Set the message used to remove the dynamic layer
//
void QmlChartViewer::removeDynamicLayer(const char *msg)
{
    m_autoHideMsg = QString(msg ? msg : "").toLower();
    if (m_autoHideMsg == "now")
        applyAutoHide(msg);
}

//
// Attempt to hide the dynamic layer using the specified message
//
void QmlChartViewer::applyAutoHide(const char *msg)
{
    if (m_autoHideMsg == msg)
    {
        if (0 != m_currentChart)
            m_currentChart->removeDynamicLayer();
        m_autoHideMsg = "";
        updateDisplay();
    }
}

void QmlChartViewer::setChart(BaseChart *c)
{
    if (m_currentChart != c)
        setImageMap("");

    m_currentChart = c;
    delete m_imageMapChart;
    m_imageMapChart = c ? new BaseChart(c) : 0;

    if (0 != c)
    {
        commitPendingSyncAxis(c);
        if (m_delayUpdateChart != NO_DELAY)
            c->makeChart();
    }

    updateDisplay();
}

BaseChart *QmlChartViewer::getChart()
{
    return m_currentChart;
}

//
// Set image map used by the chart
//
void QmlChartViewer::setImageMap(const char *imageMap)
{
    //delete the existing ImageMapHandler
    if (0 != m_hotSpotTester)
        delete m_hotSpotTester;
    m_currentHotSpot = -1;
    m_isClickable = false;

    //create a new ImageMapHandler to represent the image map
    if ((0 == imageMap) || (0 == *imageMap))
        m_hotSpotTester = 0;
    else
        m_hotSpotTester = new ImageMapHandler(imageMap, this);
}

//
// Get the image map handler for the chart
//
ImageMapHandler *QmlChartViewer::getImageMapHandler()
{
    return m_hotSpotTester;
}

QVariant QmlChartViewer::getHotSpot()
{
    ImageMapHandler *h = getImageMapHandler();
    if (0 == h)
        return QVariant();

    QVariantMap ret;

    // Query the ImageMapHandler to see if the mouse is on a clickable hot spot. We
    // consider the hot spot as clickable if its href ("path") parameter is not empty.
    const char *path = h->getValue("path");
    if ((0 != path) && (0 != *path))
    {
        for (int i = 0; 0 != h->getKey(i); ++i)
            ret[QString::fromUtf8(h->getKey(i))] = QString::fromUtf8(h->getValue(i));
    }

    return ret;
}

//
// Set the default tool tip to use
//
void QmlChartViewer::setDefaultToolTip(const QString& text)
{
    m_defaultToolTip = text;
}

//
// Set the CDML tool tip prefix
//
void QmlChartViewer::setCDMLToolTipPrefix(const QString& prefix)
{
    m_CDMLToolTipPrefix = prefix;
}

//
// Set the border width of the selection box
//
void QmlChartViewer::setSelectionBorderWidth(int width)
{
    m_selectBoxLineWidth = width;
}

//
// Get the border with of the selection box.
//
int QmlChartViewer::getSelectionBorderWidth()
{
    return m_selectBoxLineWidth;
}

//
// Set the border color of the selection box
//
void QmlChartViewer::setSelectionBorderColor(QColor c)
{
    m_selectBoxLineColor = c;
}

//
// Get the border color of the selection box.
//
QColor QmlChartViewer::getSelectionBorderColor()
{
    return m_selectBoxLineColor;
}

//
// Set the mouse usage mode
//
void QmlChartViewer::setMouseUsage(MouseUsage mouseUsage)
{
    if (m_mouseUsage != mouseUsage)
    {
        m_mouseUsage = mouseUsage;
        emit mouseUsageChanged(m_mouseUsage);
    }
}

//
// Get the mouse usage mode
//
QmlChartViewer::MouseUsage QmlChartViewer::getMouseUsage()
{
    return m_mouseUsage;
}

//
// Set the zoom direction
//
void QmlChartViewer::setZoomDirection(Direction direction)
{
    m_zoomDirection = direction;
    setKeepAspectRatio(m_zoomDirection == Chart::KeepAspectRatio);
}

//
// Get the zoom direction
//
QmlChartViewer::Direction QmlChartViewer::getZoomDirection()
{
    return (Direction)m_zoomDirection;
}

//
// Set the scroll direction
//
void QmlChartViewer::setScrollDirection(Direction direction)
{
    m_scrollDirection = direction;
}

//
// Get the scroll direction
//
QmlChartViewer::Direction QmlChartViewer::getScrollDirection()
{
    return (Direction)m_scrollDirection;
}

//
// Set the zoom-in ratio for mouse click zoom-in
//
void QmlChartViewer::setZoomInRatio(double ratio)
{
    m_zoomInRatio = ratio;
}

//
// Get the zoom-in ratio for mouse click zoom-in
//
double QmlChartViewer::getZoomInRatio()
{
    return m_zoomInRatio;
}

//
// Set the zoom-out ratio
//
void QmlChartViewer::setZoomOutRatio(double ratio)
{
    m_zoomOutRatio = ratio;
}

//
// Get the zoom-out ratio
//
double QmlChartViewer::getZoomOutRatio()
{
    return m_zoomOutRatio;
}

//
// Set the mouse wheel zoom ratio
//
void QmlChartViewer::setMouseWheelZoomRatio(double ratio)
{
    m_mouseWheelZoomRatio = ratio;
}

//
// Get the mouse wheel zoom ratio
//
double QmlChartViewer::getMouseWheelZoomRatio()
{
    return m_mouseWheelZoomRatio;
}

//
// Set the minimum mouse drag before the dragging is considered as real. This is to avoid small
// mouse vibrations triggering a mouse drag.
//
void QmlChartViewer::setMinimumDrag(int offset)
{
    m_minDragAmount = offset;
}

//
// Get the minimum mouse drag before the dragging is considered as real.
//
int QmlChartViewer::getMinimumDrag()
{
    return m_minDragAmount;
}

//
// Set the minimum interval between ViewPortChanged events. This is to avoid the chart being
// updated too frequently. (Default is 20ms between chart updates.) Multiple update events
// arrived during the interval will be merged into one chart update and executed at the end
// of the interval.
//
void QmlChartViewer::setUpdateInterval(int interval)
{
    m_updateInterval = interval;
}

//
// Get the minimum interval between ViewPortChanged events.
//
int QmlChartViewer::getUpdateInterval()
{
    return m_updateInterval;
}

//
// Check if there is a pending chart update request.
//
bool QmlChartViewer::needUpdateChart()
{
    return m_needUpdateChart;
}

//
// Check if there is a pending image map update request.
//
bool QmlChartViewer::needUpdateImageMap()
{
    return m_needUpdateImageMap;
}

//
// Get the current mouse x coordinate when used in a mouse move event handler
//
int QmlChartViewer::getChartMouseX()
{
    if (m_currentMouseX < -0x10000000)
        return getPlotAreaLeft() + getPlotAreaWidth();
    else
        return (int)(toImageX(m_currentMouseX) + 0.5);
}

//
// Get the current mouse y coordinate when used in a mouse move event handler
//
int QmlChartViewer::getChartMouseY()
{
    if (m_currentMouseY < -0x10000000)
        return getPlotAreaTop() + getPlotAreaHeight();
    else
        return (int)(toImageY(m_currentMouseY) + 0.5);
}

//
// Get the current mouse x coordinate bounded to the plot area when used in a mouse event handler
//
int QmlChartViewer::getPlotAreaMouseX()
{
    int ret = getChartMouseX();
    if (ret < getPlotAreaLeft())
        ret = getPlotAreaLeft();
    if (ret > getPlotAreaLeft() + getPlotAreaWidth())
        ret = getPlotAreaLeft() + getPlotAreaWidth();
    return ret;
}

//
// Get the current mouse y coordinate bounded to the plot area when used in a mouse event handler
//
int QmlChartViewer::getPlotAreaMouseY()
{
    int ret = getChartMouseY();
    if (ret < getPlotAreaTop())
        ret = getPlotAreaTop();
    if (ret > getPlotAreaTop() + getPlotAreaHeight())
        ret = getPlotAreaTop() + getPlotAreaHeight();
    return ret;
}

//
// Check if mouse is on the extended plot area
//
bool QmlChartViewer::isMouseOnPlotArea()
{
    if (m_isMouseTracking)
        return inExtendedPlotArea(getChartMouseX(), getChartMouseY());
    else
        return false;
}

//
// Check if mouse is dragging to scroll or to select the zoom rectangle
//
bool QmlChartViewer::isMouseDragging()
{
    return m_isPlotAreaMouseDown;
}

//
// Check if is currently processing a mouse move event
//
bool QmlChartViewer::isInMouseMoveEvent()
{
    return m_isInMouseMove;
}

//
// Check if is currently processing a view port changed event
//
bool QmlChartViewer::isInViewPortChangedEvent()
{
    return m_isInViewPortChanged;
}

//
// Change the mouse cursor.
//
void QmlChartViewer::onSetCursor()
{
    if (this->m_isDragScrolling)
    {
        switch (m_scrollDirection)
        {
        case Chart::DirectionHorizontal:
            //setCursor(getNoMoveHorizCursor());
            setCursor(Qt::SizeHorCursor);
            break;
        case Chart::DirectionVertical:
            //setCursor(getNoMoveVertCursor());
            setCursor(Qt::SizeVerCursor);
            break;
        default :
            //setCursor(getNoMove2DCursor());
            setCursor(Qt::SizeAllCursor);
            break;
        }

        return;
    }

    if (m_isOnPlotArea)
    {
        QQuickWindow *w = window();
        double devicePixelRatio = (w ? w->effectiveDevicePixelRatio() : 1);

        switch (m_mouseUsage)
        {
        case MouseUsageZoomIn:
            if (canZoomIn(m_zoomDirection))
                setCursor(cursorManager.getZoomInCursor(devicePixelRatio));
            else
                setCursor(cursorManager.getNoZoomCursor(devicePixelRatio));
            return;
        case MouseUsageZoomOut:
            if (canZoomOut(m_zoomDirection))
                setCursor(cursorManager.getZoomOutCursor(devicePixelRatio));
            else
                setCursor(cursorManager.getNoZoomCursor(devicePixelRatio));
            return;
        default:
            // do nothing
            ;
        }
    }

    if (m_isClickable)
        setCursor(Qt::PointingHandCursor);
    else
        unsetCursor();
}

//
// Mouse button down event.
//
void QmlChartViewer::mousePressEvent(QMouseEvent *event)
{
    onDelayedMouseMove();
    if ((event->button() == Qt::LeftButton) && inPlotArea(toImageX(event->x()), toImageY(event->y())) &&
        (m_mouseUsage != MouseUsageDefault))
    {
        // Mouse usage is for drag to zoom/scroll. Capture the mouse to prepare for dragging and
        // save the mouse down position to draw the selection rectangle.
        m_isPlotAreaMouseDown = true;
        m_plotAreaMouseDownXPos = event->x();
        m_plotAreaMouseDownYPos = event->y();
        startDrag();
    }

    emit mousePressed(event->button());
}

//
// Mouse button up event.
//
void QmlChartViewer::mouseReleaseEvent(QMouseEvent *event)
{
    onDelayedMouseMove();

    if ((event->button() == Qt::LeftButton) && m_isPlotAreaMouseDown)
    {
        // Release the mouse capture.
        m_isPlotAreaMouseDown = false;
        setRectVisible(false);
        bool hasUpdate = false;

        switch (m_mouseUsage)
        {
        case MouseUsageZoomIn :
            if (canZoomIn(m_zoomDirection))
            {
                if (isDrag(m_zoomDirection, event))
                {
                    int minX, minY, spanX, spanY;
                    getDragZoomRect(event->x(), event->y(), &minX, &minY, &spanX, &spanY);

                    // Zoom to the drag selection rectangle.
                    hasUpdate = zoomTo(m_zoomDirection,
                        toImageX(minX), toImageY(minY), toImageX(minX + spanX), toImageY(minY + spanY));
                }
                else
                    // User just click on a point. Zoom-in around the mouse cursor position.
                    hasUpdate = zoomAt(m_zoomDirection, toImageX(event->x()), toImageY(event->y()), m_zoomInRatio);
            }
            break;
        case MouseUsageZoomOut:
            // Zoom out around the mouse cursor position.
            if (canZoomOut(m_zoomDirection))
                hasUpdate = zoomAt(m_zoomDirection, toImageX(event->x()), toImageY(event->y()), m_zoomOutRatio);
            break;
        default :
            if (m_isDragScrolling)
                // Drag to scroll. We can update the image map now as scrolling has finished.
                updateViewPort(false, true);
            else
                // Is not zooming or scrolling, so is just a normal click event.
                emit clicked(event->button());
            break;
        }

        m_isDragScrolling = false;
        if (hasUpdate)
            // View port has changed - update it.
            updateViewPort(true, true);
    }
    else
        emit clicked(event->button());

    onSetCursor();

    emit mouseReleased(event->button());
}

void QmlChartViewer::mouseMoveEvent(QMouseEvent *event)
{
    if ((m_currentMouseX == event->x()) && (m_currentMouseY == event->y()))
        return;

    // Enable mouse tracking to detect mouse leave events
    m_isMouseTracking = true;

    emit mouseMove();

    // On Windows, mouse events can by-pass the event queue. If there are too many mouse events,
    // the event queue may not get processed, preventing other controls from updating. If two mouse
    // events are less than 10ms apart, there is a risk of too many mouse events. So we repost the
    // mouse event as a timer event that is queued up normally, allowing the queue to get processed.
    unsigned int timeBetweenMouseMove = (((unsigned int)clock()) - m_lastMouseMove)
                                        * 1000 / CLOCKS_PER_SEC ;
    if ((m_delayedMouseEvent && (timeBetweenMouseMove < 250)) || (timeBetweenMouseMove < 10))
    {
        if (!m_delayedMouseEvent)
            m_delayedMouseEventTimerId = startTimer(1);
        else
            delete m_delayedMouseEvent;

        m_delayedMouseEvent = new QMouseEvent(event->type(), event->pos(), event->globalPos(),
            event->button(), event->buttons(), event->modifiers());
    }
    else
        commitMouseMove(event);

    onSetCursor();
}

//
// The method that actually performs MouseMove event processing
//
void QmlChartViewer::commitMouseMove(QMouseEvent *event)
{
    // Remember the mouse coordinates for later use
    m_currentMouseX = event->x();
    m_currentMouseY = event->y();

    // The chart can be updated more than once during mouse move. For example, it can update due to
    // drag to scroll, and also due to drawing track cursor. So we delay updating the display until
    // all all events has occured.
    m_delayUpdateChart = NEED_DELAY;
    m_isInMouseMove = true;

    // Check if mouse is dragging on the plot area
    m_isOnPlotArea = m_isPlotAreaMouseDown || inPlotArea(toImageX(event->x()), toImageY(event->y()));
    if (m_isPlotAreaMouseDown)
        onPlotAreaMouseDrag(event);

    // Emit mouseMoveChart
    emit mouseMoveChart();

    if (inExtendedPlotArea(toImageX(event->x()), toImageY(event->y())))
    {
        // Mouse is in extended plot area, emit mouseMovePlotArea
        m_isInMouseMovePlotArea = true;
        emit mouseMovePlotArea();
    }
    else if (m_isInMouseMovePlotArea)
    {
        // Mouse was in extended plot area, but is not in it now, so emit mouseLeavePlotArea
        m_isInMouseMovePlotArea = false;
        emit mouseLeavePlotArea();
        applyAutoHide("mouseleaveplotarea");
    }

    // Can update chart now
    commitUpdateChart();
    m_isInMouseMove = false;

    if (m_delayImageMapUpdate)
    {
        m_delayImageMapUpdate = false;
        if (!m_isPlotAreaMouseDown)
            updateViewPort(false, true);
    }

    // Use the ChartDirector ImageMapHandler to determine if the mouse is over a hot spot
    int hotSpotNo = 0;
    if (0 != m_hotSpotTester)
        hotSpotNo = m_hotSpotTester->getHotSpot(toImageX(event->x()), toImageY(event->y()), m_imageMapChart);

    const char* isDynamic = 0;
    if (hotSpotNo != 0)
        isDynamic = m_hotSpotTester->getValue("dynamic");   

    // If the mouse is in the same hot spot since the last mouse move event, there is no need
    // to update the tool tip.
    if ((hotSpotNo != m_currentHotSpot) || ((0 != isDynamic) && (0 != *isDynamic)))
    {
        // Hot spot has changed - update tool tip text
        m_currentHotSpot = hotSpotNo;

        if (hotSpotNo == 0)
        {
            // Mouse is not on hot spot - use default tool tip text and reset
            // the clickable flag.
            m_isClickable = false;
            showToolTip(event->pos(), m_defaultToolTip);
        }
        else
        {
            // Mouse is on a hot spot. In this implementation, we consider the hot spot as
            // clickable if its href ("path") parameter is not empty.
            const char *path = m_hotSpotTester->getValue("path");
            m_isClickable = ((0 != path) && (0 != *path) && ('~' != *path));
            showToolTip(event->pos(), QString::fromUtf8(m_hotSpotTester->getValue("title")));
        }
    }
    else
        moveToolTip(event->pos());

    // Cancel the delayed mouse event if any
    if (m_delayedMouseEvent)
    {
        killTimer(m_delayedMouseEventTimerId);
        delete m_delayedMouseEvent;
        m_delayedMouseEvent = 0;
    }

    m_lastMouseMove = (unsigned int)clock();
}

// In QML, the moveMoveEvent only occurs if there mouse button is pressed while the
// mouse move. Otherwise, the hoverMoveEvent occurs.
void QmlChartViewer::hoverMoveEvent(QHoverEvent* event)
{
    QMouseEvent e(QEvent::MouseMove, QPointF(event->pos()), Qt::MouseButton::NoButton,
                  Qt::MouseButtons(), Qt::KeyboardModifier::NoModifier);
    mouseMoveEvent(&e);
}

//
// Chart hold timer.
//
void QmlChartViewer::timerEvent(QTimerEvent *event)
{
    if (m_delayedMouseEvent && (event->timerId() == m_delayedMouseEventTimerId))
    {
        // Is a delayed mouse move event
        onDelayedMouseMove();
    }
    else if (m_holdTimerActive && (event->timerId() == m_holdTimerId))
    {
        killTimer(m_holdTimerId);
        m_holdTimerActive = false;

        // If has pending chart update request, handles them now.
        if (m_needUpdateChart || m_needUpdateImageMap)
            updateViewPort(m_needUpdateChart, m_needUpdateImageMap);
    }
}

//
// Delayed MouseMove event handler
//
void QmlChartViewer::onDelayedMouseMove()
{
    if (m_delayedMouseEvent)
        commitMouseMove(m_delayedMouseEvent);
}

void QmlChartViewer::showToolTip(QPoint pos, QString text)
{      
    if (!(text.isEmpty() || text.startsWith("<*")))
        text = "<*cdml*>" + text;
    text.replace("<*cdml*>", m_CDMLToolTipPrefix);
    setToolTip(pos, text);
}

//
// Mouse Leave event handler
//
void QmlChartViewer::hoverLeaveEvent(QHoverEvent* event)
{
    // Process delayed mouse move, if any
    onDelayedMouseMove();

    // Mouse tracking is no longer active
    m_isMouseTracking = false;

    if (m_isInMouseMovePlotArea)
    {
        // Mouse was in extended plot area, but is not in it now, so emit mouseLeavePlotArea
        m_isInMouseMovePlotArea = false;
        emit mouseLeavePlotArea();
        applyAutoHide("mouseleaveplotarea");
    }

    emit mouseLeaveChart();
    applyAutoHide("mouseleavechart");

    // hide tooltip
    m_currentHotSpot = -1;
    showToolTip(QPoint(0, 0), "");
}

//
// Mouse wheel event handler
//
void QmlChartViewer::wheelEvent(QWheelEvent *event)
{
    bool hasReceivers = receivers(SIGNAL(mouseWheel(QPoint))) > 0;
    if (hasReceivers)
    {
        // Process delayed mouse move, if any
        onDelayedMouseMove();

        // emit mouseWheel event
        emit mouseWheel(event->angleDelta());
    }

    // Process the mouse wheel only if the mouse is over the plot area
    bool hasMouseWheelZoom = isMouseOnPlotArea() &&
        onMouseWheelZoom(getPlotAreaMouseX(), getPlotAreaMouseY(), event->angleDelta().y());

    if (!(hasReceivers || hasMouseWheelZoom))
        event->ignore();
}

bool QmlChartViewer::onMouseWheelZoom(int x, int y, int zDelta)
{
    // Zoom ratio = 1 means no zooming
    if (m_mouseWheelZoomRatio == 1)
        return false;

    // X and Y zoom ratios
    double rx = 1;
    double ry = 1;
    if (getZoomDirection() != Chart::DirectionVertical)
        rx = (zDelta > 0) ? m_mouseWheelZoomRatio : 1 / m_mouseWheelZoomRatio;
    if (getZoomDirection() != Chart::DirectionHorizontal)
        ry = (zDelta > 0) ? m_mouseWheelZoomRatio : 1 / m_mouseWheelZoomRatio;

    // Do the zooming
    if (zoomAround(x, y, rx, ry))
        updateViewPort(true, true);

    return true;
}

//
// Set selection rectangle position and size
//
void QmlChartViewer::drawRect(int x, int y, int width, int height)
{
    // width < 0 is interpreted as the rectangle extends to the left or x.
    // height <0 is interpreted as the rectangle extends to above y.
    if (width < 0)
        x -= (width = -width);
    if (height < 0)
        y -= (height = -height);

    setRect(x, y, width, height, m_selectBoxLineColor, m_selectBoxLineWidth);
}

//
// Show/hide selection rectangle
//
void QmlChartViewer::setRectVisible(bool b)
{
    if (!b)
        setRect(0, 0, 0, 0, QColor(0, 0, 0, 0), 0);
}

//
// Determines if the mouse is dragging.
//
bool QmlChartViewer::isDrag(int direction, QMouseEvent* event)
{
    // We only consider the mouse is dragging it is has dragged more than m_minDragAmount. This is
    // to avoid small mouse vibrations triggering a mouse drag.
    int spanX = abs(event->pos().x() - m_plotAreaMouseDownXPos);
    int spanY = abs(event->pos().y() - m_plotAreaMouseDownYPos);
    return ((direction != Chart::DirectionVertical) && (spanX >= m_minDragAmount)) ||
        ((direction != Chart::DirectionHorizontal) && (spanY >= m_minDragAmount));
}

void QmlChartViewer::getDragZoomRect(int px, int py, int* x, int* y, int* w, int* h)
{
    *x = (px < m_plotAreaMouseDownXPos) ? px : m_plotAreaMouseDownXPos;
    *y = (py < m_plotAreaMouseDownYPos) ? py : m_plotAreaMouseDownYPos;
    *w = abs(px - m_plotAreaMouseDownXPos);
    *h = abs(py - m_plotAreaMouseDownYPos);

    if (getZoomXYRatio() > 0)
    {
        double imageXYRatio = getPlotAreaWidth() / (double)getPlotAreaHeight();

        double whDelta = toImageX(*w) - toImageY(*h) * imageXYRatio;
        if (whDelta < 0)
            *w = toDisplayX(toImageY(*h) * imageXYRatio);
        else if (whDelta > 0)
            *h = toDisplayY(toImageX(*w) / imageXYRatio);

        if (*x == px)
            *x = m_plotAreaMouseDownXPos - *w;
        if (*y == py)
            *y = m_plotAreaMouseDownYPos - *h;
    }
}

//
// Process mouse dragging over the plot area
//
void QmlChartViewer::onPlotAreaMouseDrag(QMouseEvent* event)
{
    switch (m_mouseUsage)
    {
        case MouseUsageZoomIn :
        {
            //
            // Mouse is used for zoom in. Draw the selection rectangle if necessary.
            //

            bool isDragZoom = canZoomIn(m_zoomDirection) && isDrag(m_zoomDirection, event);
            if (isDragZoom)
            {
                int minX, minY, spanX, spanY;
                getDragZoomRect(event->x(), event->y(), &minX, &minY, &spanX, &spanY);

                switch (m_zoomDirection)
                {
                case Chart::DirectionHorizontal:
                    drawRect(minX, toDisplayY(getPlotAreaTop()), spanX, toDisplayY(getPlotAreaHeight()));
                    break;
                case Chart::DirectionVertical:
                    drawRect(toDisplayX(getPlotAreaLeft()), minY, toDisplayX(getPlotAreaWidth()), spanY);
                    break;
                default:
                    drawRect(minX, minY, spanX, spanY);
                    break;
                }
            }
            setRectVisible(isDragZoom);
            break;
        }
        case MouseUsageScroll :
        {
            //
            // Mouse is used for drag scrolling. Scroll and update the view port.
            //

            if (m_isDragScrolling || isDrag(m_scrollDirection, event))
            {
                m_isDragScrolling = true;
                if (dragTo(m_scrollDirection,
                    toImageX(event->pos().x() - m_plotAreaMouseDownXPos), toImageY(event->pos().y() - m_plotAreaMouseDownYPos)))
                    updateViewPort(true, false);
            }
            break;
        }
        default:
        {
            // do nothing
        }
    }
}

//
// Trigger the ViewPortChanged event
//
void QmlChartViewer::updateViewPort(bool needUpdateChart, bool needUpdateImageMap)
{
    // Already updating, no need to update again
    if (m_isInViewPortChanged)
        return;

    // Merge the current update requests with any pending requests.
    m_needUpdateChart = m_needUpdateChart || needUpdateChart;
    m_needUpdateImageMap = needUpdateImageMap;

    // Hold timer has not expired, so do not update chart immediately. Keep the requests pending.
    if (m_holdTimerActive)
    {
        if (!m_holdElapsedTimer.hasExpired(m_updateInterval))
            return;

        m_holdTimerActive = false;
        killTimer(m_holdTimerId);
    }

    // The chart can be updated more than once during mouse move. For example, it can update due to
    // drag to scroll, and also due to drawing track cursor. So we delay updating the display until
    // all all updates has occured.
    int hasDelayUpdate = (m_delayUpdateChart != NO_DELAY);
    if (!hasDelayUpdate)
        m_delayUpdateChart = NEED_DELAY;

    // Can trigger the ViewPortChanged event.
    validateViewPort();
    m_isInViewPortChanged = true;
    emit viewPortChanged();
    m_isInViewPortChanged = false;

    // Can update chart now
    if (!hasDelayUpdate)
        commitUpdateChart();

    // Clear any pending updates.
    m_needUpdateChart = false;
    m_needUpdateImageMap = false;

    // Set hold timer to prevent multiple chart updates within a short period.
    if (m_updateInterval > 0)
    {
        m_holdTimerActive = true;
        m_holdTimerId = startTimer(m_updateInterval);
        m_holdElapsedTimer.restart();
    }
}



/////////////////////////////////////////////////////////////////////////////
//
// QViewPortControl
//
/////////////////////////////////////////////////////////////////////////////

QmlViewPortControl::QmlViewPortControl(QQuickItem*parent) : QmlChartImage(parent)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setAntialiasing(true);

    m_ChartViewer = 0;
    m_Chart = 0;

    m_mouseDownX = 0;
    m_mouseDownY = 0;
}

//
// Free	resources
//
QmlViewPortControl::~QmlViewPortControl()
{
}

//
// Set the CChartViewer to be associated with this control
//
void QmlViewPortControl::setViewer(QmlChartViewer *viewer)
{
    if (0 != m_ChartViewer)
        // disconnect from previous QChartViewer
        disconnect(m_ChartViewer, 0, this, 0);
    m_ChartViewer = viewer;
    setViewPortManager(viewer);
    if (0 != m_ChartViewer)
        // Connect to new QChartViewer
        connect(m_ChartViewer, SIGNAL(viewPortChanged()), SLOT(onViewPortChanged()));

    updateDisplay();
}

//
// Get back	the	same QChartViewer pointer provided	by the previous	setViewer call.
//
QmlChartViewer *QmlViewPortControl::getViewer()
{
    return m_ChartViewer;
}

//
// Set the chart to	be displayed in the	control
//
void QmlViewPortControl::setChart(BaseChart *c)
{
    m_Chart = c;
    ViewPortControlBase::setChart(c);
    updateDisplay();
}

//
// Get back	the	same BaseChart pointer provided	by the previous	setChart call.
//
BaseChart *QmlViewPortControl::getChart()
{
    return m_Chart;
}

//
// Display the chart
//
void QmlViewPortControl::paintDisplay()
{
    displayChart(getChart());
}

//
// Handle the ViewPortChanged event from the associated QChartViewer
//
void QmlViewPortControl::onViewPortChanged()
{
    updateDisplay();
}

//
// Mouse button down event.
//
void QmlViewPortControl::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    // Remember current mouse position
    m_mouseDownX = event->x();
    m_mouseDownY = event->y();

    // Get the QChartViewer zoom/scroll state to determine which type of mouse action is allowed
    syncState();

    // Handle the mouse down event
    handleMouseDown(toImageX(event->x()), toImageY(event->y()));

    // Update the chart viewer if the viewport has changed
    updateChartViewerIfNecessary();
}

//
// MouseMove event handler
//
void QmlViewPortControl::mouseMoveEvent(QMouseEvent *event)
{
    // Get the QChartViewer zoom/scroll state to determine which type of mouse action is allowed
    syncState();

    // Handle the mouse move event
    handleMouseMove(toImageX(event->x()), toImageY(event->y()), isDrag(event));

    // Update the chart viewer if the viewport has changed
    updateChartViewerIfNecessary();

    // Update the mouse cursor
    updateCursor(getCursor());

    // Update the display
    if (needUpdateDisplay())
        paintDisplay();
}

// In QML, the moveMoveEvent only occurs if there mouse button is pressed while the
// mouse move. Otherwise, the hoverMoveEvent occurs.
void QmlViewPortControl::hoverMoveEvent(QHoverEvent* event)
{
    QMouseEvent e(QEvent::MouseMove, QPointF(event->pos()), Qt::MouseButton::NoButton,
                  Qt::MouseButtons(), Qt::KeyboardModifier::NoModifier);
    mouseMoveEvent(&e);
}

//
// Mouse button up event.
//
void QmlViewPortControl::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    // Get the QChartViewer zoom/scroll state to determine which type of mouse action is allowed
    syncState();

    // Handle the mouse up event
    handleMouseUp(toImageX(event->x()), toImageY(event->y()));

    // Update the chart viewer if the viewport has changed
    updateChartViewerIfNecessary();
}

//
// MouseWheel handler
//
void QmlViewPortControl::wheelEvent(QWheelEvent *event)
{
#if QT_VERSION >= 0x051400
    double wx = event->position().x();
    double wy = event->position().y();
#else
    double wx = event->x();
    double wy = event->y();
#endif

    // Process the mouse wheel only if the mouse is over the plot area
    if ((0 == m_ChartViewer) || (!isOnPlotArea(wx, wy)))
        event->ignore();
    else
    {
        // Ask the CChartViewer to zoom around the center of the chart
        int x = m_ChartViewer->getPlotAreaLeft() + m_ChartViewer->getPlotAreaWidth() / 2;
        int y = m_ChartViewer->getPlotAreaTop() + m_ChartViewer->getPlotAreaHeight() / 2;
        if (!m_ChartViewer->onMouseWheelZoom(x, y, event->angleDelta().y()))
            event->ignore();
    }
}

//
// Get the QChartViewer zoom/scroll state
//
void QmlViewPortControl::syncState()
{
    QmlChartViewer *viewer = getViewer();
    if (0 != viewer)
        setZoomScrollDirection(viewer->getZoomDirection(), viewer->getScrollDirection());
}

//
// Determines if the mouse is dragging.
//
bool QmlViewPortControl::isDrag(QMouseEvent *event)
{
    QmlChartViewer *viewer = getViewer();
    if (0 == viewer)
        return false;

    int minimumDrag = viewer->getMinimumDrag();
    int moveX = abs(m_mouseDownX - event->x());
    int moveY = abs(m_mouseDownY - event->y());
    return (moveX >= minimumDrag) || (moveY >= minimumDrag);
}

//
// Update the display
//
void QmlViewPortControl::updateDisplay()
{
    paintViewPort();
    paintDisplay();
}

//
// Update the QChartViewer if the viewport has changed
//
void QmlViewPortControl::updateChartViewerIfNecessary()
{
    QmlChartViewer *viewer = getViewer();
    if (0 == viewer)
        return;

    if (needUpdateChart() || needUpdateImageMap())
        viewer->updateViewPort(needUpdateChart(), needUpdateImageMap());
}

//
// Update the mouse cursor
//
void QmlViewPortControl::updateCursor(int position)
{
    switch (position)
    {
    case Chart::Left:
    case Chart::Right:
        setCursor(Qt::SizeHorCursor);
        return;
    case Chart::Top:
    case Chart::Bottom:
        setCursor(Qt::SizeVerCursor);
        return;
    case Chart::TopLeft:
    case Chart::BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        return;
    case Chart::TopRight:
    case Chart::BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        return;
    default:
        unsetCursor();
        return;
    }
}









