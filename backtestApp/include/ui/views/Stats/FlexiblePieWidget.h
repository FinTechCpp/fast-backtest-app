#pragma once

#include "ui/views/Stats/TitledWidget.h"
#include <vector>

// Structure to define a segment
struct PieSegment {
    double proportion;  // Relative proportion (will be normalized)
    QColor color;       // Segment color
    
    PieSegment(double p, const QColor& c) : proportion(p), color(c) {}
};

class FlexiblePieWidget : public TitledWidget
{
    Q_OBJECT

public:
    explicit FlexiblePieWidget(const QString& title = QString(), QWidget *parent = nullptr);
    
    // Basic configuration
    void setStartAngle(int degrees); // 0° = east, 90° = top, 180° = west
    void setAngleSpan(int degrees);   // 180° = half-circle, 360° = full circle
    void setCenterText(const QString &text);
    void setCenterTextColor(const QColor &color);
    void setCenterTextSuffix(const QString &suffix);
    
    // Segment management
    void addSegment(double proportion, const QColor &color);
    void setSegments(const std::vector<PieSegment>& segments);
    void clearSegments();
    const std::vector<PieSegment>& segments() const { return m_segments; }
    
    // Visual configuration
    void setInnerCircleRadius(double ratio); // 0.1-0.9, proportion of the outer radius
    
    // Recommended size
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintContent(QPainter& painter, const QRect& contentRect) override;

private:
    // Geometric configuration
    int m_startAngle;    // Starting angle in degrees (x16 for QPainter)
    int m_angleSpan;      // Maximum angle to cover (x16 for QPainter)
    double m_innerRadiusRatio; // Ratio of the inner radius
    
    // Center text
    QString m_centerText;
    QString m_textSuffix;
    QColor m_textColor;
    
    // Segments
    std::vector<PieSegment> m_segments;
};