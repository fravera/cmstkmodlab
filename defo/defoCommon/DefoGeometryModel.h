#ifndef DEFOGEOMETRYMODEL_H
#define DEFOGEOMETRYMODEL_H

#include <QObject>

class DefoGeometryModel : public QObject
{
    Q_OBJECT
public:
  explicit DefoGeometryModel(QObject *parent = 0);

  void setAngle1(double v);
  void setAngle2(double v);
  void setAngle3(double v);
  void setDistance(double v);
  void setHeight1(double v);
  void setHeight2(double v);

  void setCalibX(double v);
  void setCalibY(double v);

  double getAngle1() const { return angle1_; }
  double getAngle2() const { return angle2_; }
  double getAngle3() const { return angle3_; }
  double getDistance() const { return distance_; }
  double getHeight1() const { return height1_; }
  double getHeight2() const { return height2_; }

  double getCalibX() const { return calibX_; }
  double getCalibY() const { return calibY_; }

  void write(const QString& filename);
  void read(const QString& filename);

public slots:

protected:

  double angle1_;
  double angle2_;
  double angle3_;
  double distance_;
  double height1_;
  double height2_;

  double calibX_;
  double calibY_;

signals:

  void geometryChanged();
};

#endif // DEFOGEOMETRYMODEL_H
