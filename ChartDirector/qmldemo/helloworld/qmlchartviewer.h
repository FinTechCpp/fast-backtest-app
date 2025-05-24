#ifndef QMLCHARTVIEWER_H
#define QMLCHARTVIEWER_H

#include <QQuickPaintedItem>
#include <QPainter>
#include <QElapsedTimer>
#include "chartdir.h"


class QmlChartImage : public QQuickPaintedItem
{
    Q_OBJECT

public:
    explicit QmlChartImage(QQuickItem *parent = 0);
    virtual ~QmlChartImage();

    //
    // High DPI support
    //
    virtual void setDpi(int dpi);
    virtual int getDpi();

    virtual double toImageX(int x);
    virtual double toImageY(int y);
    virtual int toDisplayX(double x);
    virtual int toDisplayY(double y);

protected:
    virtual void paint(QPainter *painter) Q_DECL_OVERRIDE;

    // Chart Image
    QPixmap m_chartImage;
    virtual void displayChart(BaseChart *c);

    // Selection rectangle
    QRect m_rect;
    QColor m_lineColor;
    int m_lineWidth;
    virtual void setRect(int x, int y, int w, int h, QColor lineColor, int lineWidth);

    QString m_toolTip;
    QPoint m_toolTipPos;
    QPixmap m_toolTipImage;
    virtual void setToolTip(QPoint pos, QString tooltip);
    virtual void moveToolTip(QPoint pos);

private:   
    int m_dpi;
    double m_toImageScaleX;
    double m_toImageScaleY;

    QPixmap makePixmap(BaseChart *c);
};


class QmlChartViewer : public QmlChartImage, public ViewPortManager
{
    Q_OBJECT

public:
    enum MouseUsage
    {
        MouseUsageDefault = 0,
        MouseUsageScroll = 1,
        MouseUsageZoomIn = 3,
        MouseUsageZoomOut = 4
    };
    Q_ENUM(MouseUsage)

    enum Direction
    {
        Horizontal = 0,
        Vertical = 1,
        HorizontalVertical = 2,
        KeepAspectRatio = 3
    };
    Q_ENUM(Direction)

private:

    Q_PROPERTY(MouseUsage mouseUsage READ getMouseUsage WRITE setMouseUsage NOTIFY mouseUsageChanged)
    Q_PROPERTY(Direction zoomDirection READ getZoomDirection WRITE setZoomDirection)
    Q_PROPERTY(Direction scrollDirection READ getScrollDirection WRITE setScrollDirection)
    Q_PROPERTY(double zoomInRatio READ getZoomInRatio WRITE setZoomInRatio)
    Q_PROPERTY(double zoomOutRatio READ getZoomOutRatio WRITE setZoomOutRatio)
    Q_PROPERTY(double zoomInWidthLimit READ getZoomInWidthLimit WRITE setZoomInWidthLimit)
    Q_PROPERTY(double zoomInHeightLimit READ getZoomInHeightLimit WRITE setZoomInHeightLimit)
    Q_PROPERTY(double zoomOutWidthLimit READ getZoomOutWidthLimit WRITE setZoomOutWidthLimit)
    Q_PROPERTY(double zoomOutHeightLimit READ getZoomOutHeightLimit WRITE setZoomOutHeightLimit)
    Q_PROPERTY(double mouseWheelZoomRatio READ getMouseWheelZoomRatio WRITE setMouseWheelZoomRatio)
    Q_PROPERTY(int selectionBorderWidth READ getSelectionBorderWidth WRITE setSelectionBorderWidth)
    Q_PROPERTY(QColor selectionBorderColor READ getSelectionBorderColor WRITE setSelectionBorderColor)
    Q_PROPERTY(int minimumDrag READ getMinimumDrag WRITE setMinimumDrag)
    Q_PROPERTY(int updateInterval READ getUpdateInterval WRITE setUpdateInterval)
    Q_PROPERTY(int chartMouseX READ getChartMouseX)
    Q_PROPERTY(int chartMouseY READ getChartMouseY)
    Q_PROPERTY(int plotAreaMouseX READ getPlotAreaMouseX)
    Q_PROPERTY(int plotAreaMouseY READ getPlotAreaMouseY)
    Q_PROPERTY(int plotAreaLeft READ getPlotAreaLeft)
    Q_PROPERTY(int plotAreaTop READ getPlotAreaTop)
    Q_PROPERTY(int plotAreaWidth READ getPlotAreaWidth)
    Q_PROPERTY(int plotAreaHeight READ getPlotAreaHeight)
    Q_PROPERTY(double viewPortLeft READ getViewPortLeft WRITE setViewPortLeft)
    Q_PROPERTY(double viewPortRight READ getViewPortRight)
    Q_PROPERTY(double viewPortTop READ getViewPortTop WRITE setViewPortTop)
    Q_PROPERTY(double viewPortBottom READ getViewPortBottom)
    Q_PROPERTY(double viewPortWidth READ getViewPortWidth WRITE setViewPortWidth)
    Q_PROPERTY(double viewPortHeight READ getViewPortHeight WRITE setViewPortHeight)
    Q_PROPERTY(bool isMouseOnPlotArea READ isMouseOnPlotArea)
    Q_PROPERTY(bool isInMouseMoveEvent READ isInMouseMoveEvent)
    Q_PROPERTY(bool isInViewPortChangedEvent READ isInViewPortChangedEvent)
    Q_PROPERTY(bool isMouseDragging READ isMouseDragging)
    Q_PROPERTY(bool needUpdateChart READ needUpdateChart)
    Q_PROPERTY(bool needUpdateImageMap READ needUpdateImageMap)
    Q_PROPERTY(QString cdmlToolTipPrefix MEMBER m_CDMLToolTipPrefix)
    Q_PROPERTY(QString defaultToolTip MEMBER m_defaultToolTip)

public:

    QmlChartViewer(QQuickItem *parent = Q_NULLPTR);
    ~QmlChartViewer();

    //
    // QmlChartViewer properties
    //

    virtual void setChart(BaseChart *c);
    virtual BaseChart* getChart();

    virtual void setImageMap(const char *imageMap);
    virtual ImageMapHandler *getImageMapHandler();
    Q_INVOKABLE QVariant getHotSpot();

    virtual void setDefaultToolTip(const QString& text);
    virtual void setCDMLToolTipPrefix(const QString& text);

    virtual void setSelectionBorderWidth(int width);
    virtual int getSelectionBorderWidth();

    virtual void setSelectionBorderColor(QColor c);
    virtual QColor getSelectionBorderColor();

    virtual void setMouseUsage(MouseUsage mouseUsage);
    virtual MouseUsage getMouseUsage();

    virtual void setZoomDirection(Direction direction);
    virtual Direction getZoomDirection();

    virtual void setScrollDirection(Direction direction);
    virtual Direction getScrollDirection();

    virtual void setZoomInRatio(double ratio);
    virtual double getZoomInRatio();

    virtual void setZoomOutRatio(double ratio);
    virtual double getZoomOutRatio();

    virtual void setMouseWheelZoomRatio(double ratio);
    virtual double getMouseWheelZoomRatio();

    virtual void setMinimumDrag(int offset);
    virtual int getMinimumDrag();

    virtual void setUpdateInterval(int interval);
    virtual int getUpdateInterval();

    virtual bool needUpdateChart();
    virtual bool needUpdateImageMap();

    virtual bool isMouseOnPlotArea();
    virtual bool isInMouseMoveEvent();
    virtual bool isInViewPortChangedEvent();
    virtual bool isMouseDragging();

    //
    // QmlChartViewer methods
    //

    Q_INVOKABLE virtual double getValueAtViewPort(QString id, double vpCoor, bool isLogScale = false)
    { return ViewPortManager::getValueAtViewPort(id.toUtf8().constData(), vpCoor, isLogScale); }
    Q_INVOKABLE virtual double getViewPortAtValue(QString id, double value, bool isLogScale = false)
    { return ViewPortManager::getViewPortAtValue(id.toUtf8().constData(), value, isLogScale); }

    // Trigger the ViewPortChanged event
    Q_INVOKABLE virtual void updateViewPort(bool needUpdateChart, bool needUpdateImageMap);

    // Request CChartViewer to redisplay the chart
    virtual void updateDisplay();

    // Handles mouse wheel zooming
    virtual bool onMouseWheelZoom(int x, int y, int zDelta);

    // Set the message used to remove the dynamic layer
    virtual void removeDynamicLayer(const char *msg);

    // Get the mouse coordinates
    virtual int getChartMouseX();
    virtual int getChartMouseY();

    // Get the mouse coordinates bounded to the plot area
    virtual int getPlotAreaMouseX();
    virtual int getPlotAreaMouseY();

    // Utility to obtain the viewport right/top
    virtual double getViewPortRight() { return getViewPortLeft() + getViewPortWidth(); }
    virtual double getViewPortBottom() { return getViewPortTop() + getViewPortHeight(); }

protected:   
    //
    // QmlChartViewer event handlers
    //

    virtual void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void hoverMoveEvent(QHoverEvent* event) Q_DECL_OVERRIDE;
    virtual void onDelayedMouseMove();
    virtual void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void hoverLeaveEvent(QHoverEvent* event) Q_DECL_OVERRIDE;
    virtual void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
    virtual void timerEvent (QTimerEvent *event) Q_DECL_OVERRIDE;
    virtual void onSetCursor();

private:
    //
    // QmlChartViewer configurable properties
    //

    BaseChart *m_currentChart;          // Current BaseChart object
    QString m_defaultToolTip;           // Default tool tip text
    QString m_CDMLToolTipPrefix;        // Default prefix for CDML tooltip
    QColor m_selectBoxLineColor;        // Selectiom box border color
    int m_selectBoxLineWidth;           // Selectiom box border width
    MouseUsage m_mouseUsage;            // Mouse usage mode
    int m_zoomDirection;                // Zoom direction
    int m_scrollDirection;              // Scroll direction
    double m_zoomInRatio;               // Click zoom in ratio
    double m_zoomOutRatio;              // Click zoom out ratio
    double m_mouseWheelZoomRatio;       // Mouse wheel zoom ratio
    int m_minDragAmount;                // Minimum drag amount
    int m_updateInterval;               // Minimum interval between chart updates
    bool m_needUpdateChart;             // Has pending chart update request
    bool m_needUpdateImageMap;          // Has pending image map udpate request

    //
    // Keep track of mouse states
    //
    int m_currentHotSpot;               // The hot spot under the mouse cursor.
    bool m_isClickable;                 // Mouse is over a clickable hot spot.
    bool m_isOnPlotArea;                // Mouse is over the plot area.
    bool m_isPlotAreaMouseDown;         // Mouse left button is down in the plot area.
    bool m_isDragScrolling;             // Is current dragging scrolling the chart.
    bool m_isMouseTracking;             // Is tracking mouse leave event
    bool m_isInMouseMove;               // Is in mouse moeve event handler

    //
    // Dragging support
    //

    int m_plotAreaMouseDownXPos;        // The starting x coordinate of the mouse drag.
    int m_plotAreaMouseDownYPos;        // The starting y coordinate of the mouse drag.
    bool isDrag(int direction, QMouseEvent *event);   // Check if mouse is dragging
    void onPlotAreaMouseDrag(QMouseEvent *event);     // Process mouse dragging

    //
    // Selection rectangle
    //

    void drawRect(int x, int y, int width, int height);     // Draw selection rectangle
    void setRectVisible(bool b);        // Show/hide selection rectangle
    void getDragZoomRect(int pxc, int py, int* x, int* y, int* w, int* h);

    //
    // Chart update rate control
    //

    bool m_holdTimerActive;             // Delay chart update to limit update frequency
    int m_holdTimerId;                  // The id of the hold timer
    QElapsedTimer m_holdElapsedTimer;   // The hold elasped timer
    bool m_isInViewPortChanged;         // Flag to avoid infinite update loops

    int m_delayUpdateChart;             // Delay chart update until the mouse event is completed
    BaseChart *m_delayedChart;
    void commitUpdateChart();           // Commit updating the chart.

    unsigned int m_lastMouseMove;       // The timestamp of the last mouse move event.
    QMouseEvent *m_delayedMouseEvent;	// The delayed mouse move event.
    int m_delayedMouseEventTimerId;     // The id of the delayed mouse move timer.
    void commitMouseMove(QMouseEvent *event);     // Raise the delayed mouse move event.

    bool m_delayImageMapUpdate;			// Delay image map update until mouse moves on plot area

    //
    // Track Cursor support
    //
    int m_currentMouseX;                // Get the mouse x-pixel coordinate
    int m_currentMouseY;                // Get the mouse y-pixel coordinate
    int m_isInMouseMovePlotArea;    	// flag to indicate if is in a mouse move plot area event.

    //
    // Tool Tip Support
    //
    ImageMapHandler *m_hotSpotTester;   // ImageMapHander representing the image map
    BaseChart *m_imageMapChart;         // BaseChart object for handling dynamic map
    void showToolTip(QPoint pos, QString text);

    //
    // Dynamic Layer support
    //
    QString m_autoHideMsg;                      // The message that will trigger removing the dynamic layer.
    void applyAutoHide(const char *msg);        // Attempt to remove the dynamic layer with the given message.

signals:
    void viewPortChanged();                     // View port has changed
    void clicked(Qt::MouseButton button);       // Mouse button clicked
    void mousePressed(Qt::MouseButton button);  // Mouse button clicked
    void mouseReleased(Qt::MouseButton button); // Mouse button clicked
    void mouseMove();                           // Mouse moves over the widget
    void mouseWheel(QPoint angleDelta);         // Mouse wheel scrolled
    void mouseMoveChart();                      // Mouse moves over the chart
    void mouseLeaveChart();                     // Mouse leaves the chart
    void mouseMovePlotArea();                   // Mouse moves over the extended plot area
    void mouseLeavePlotArea();                  // Mouse leaves the extended plot area

    void mouseUsageChanged(MouseUsage mouseUsage);     // Mouse usage changed
};



class QmlViewPortControl : public QmlChartImage, public ViewPortControlBase
{
    Q_OBJECT
    Q_PROPERTY(QmlChartViewer* viewer READ getViewer WRITE setViewer)
    Q_PROPERTY(int viewPortExternalColor READ getViewPortExternalColor WRITE setViewPortExternalColor)
    Q_PROPERTY(int viewPortFillColor READ getViewPortFillColor WRITE setViewPortFillColor)
    Q_PROPERTY(int viewPortBorderColor READ getViewPortBorderColor WRITE setViewPortBorderColor)
    Q_PROPERTY(int selectionBorderColor READ getSelectionBorderColor WRITE setSelectionBorderColor)

public:
    explicit QmlViewPortControl(QQuickItem *parent = Q_NULLPTR);
    ~QmlViewPortControl();

    // Set the chart to be displayed in the control
    virtual void setChart(BaseChart *c);
    virtual BaseChart *getChart();

    // Associated QmlChartViewer
    virtual void setViewer(QmlChartViewer *viewer);
    virtual QmlChartViewer *getViewer();

    // Request the CViewPortControl to update its contents
    virtual void updateDisplay();

protected:
    //
    // QmlViewPortControl event handlers
    //

    virtual void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void hoverMoveEvent(QHoverEvent* event) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    virtual void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

private slots:
    virtual void onViewPortChanged();

private:
    QmlChartViewer *m_ChartViewer;          // Associated QmlChartViewer
    BaseChart *m_Chart;                     // BaseChart object displayed in the control

    int m_mouseDownX;                       // Current mouse x coordinates
    int m_mouseDownY;                       // Current mouse y coordinates
    bool isDrag(QMouseEvent *event);        // Check if mouse is dragging

    void paintDisplay();                    // Paint the display
    void syncState();                       // Synchronize the CViewPortControl with CChartViewer
    void updateCursor(int position);        // Update the mouse cursor
    void updateChartViewerIfNecessary();    // Update QmlChartViewer if viewport has changed

    QColor toQColor(int c) {
        if (c == Chart::Transparent)
            return QColor(0, 0, 0, 0);
        else
            return QColor((c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff, 255 - ((c >> 24) & 0xff));
    }

    int fromQColor(QColor c) {
        if (c.rgba() == 0)
            return Chart::Transparent;
        else
            return ((255 - c.alpha()) << 24) | (c.red() << 16) | (c.green() << 8) | c.blue();
    }
};


#endif // QMLCHARTVIEWER_H
