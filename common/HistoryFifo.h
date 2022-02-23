/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//               Copyright (C) 2011-2017 - The DESY CMS Group                  //
//                           All rights reserved                               //
//                                                                             //
//      The CMStkModLab source code is licensed under the GNU GPL v3.0.        //
//      You have the right to modify and/or redistribute this source code      //
//      under the terms specified in the license, which may be found online    //
//      at http://www.gnu.org/licenses or at License.txt.                      //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

#ifndef HISTORYFIFO_H
#define HISTORYFIFO_H

#include <iostream>
#include <utility>

#include <QVector>
#include <QDateTime>

/** @addtogroup common
 *  @{
 */

template <class T> class HistoryFifo
{
public:

  typedef T value_type;
  typedef size_t size_type;
  typedef std::pair<QDateTime,value_type> storage_type;

  explicit HistoryFifo(size_type n, const value_type& value = value_type())
  {
    size_ = n;
    buffer_.resize(size_);
    for (size_t i=0;i<size_;++i) {
      buffer_[i] = storage_type(QDateTime::currentDateTime(), value);
    }
  }

  void resize(size_type n, const value_type& value = value_type())
  {
    buffer_.resize(n);
    for (size_t i=size_;i<n;++i) {
      buffer_[i] = storage_type(QDateTime::currentDateTime(), value);
    }
    size_ = n;
  }

  size_type size()
  {
    return size_;
  }

  virtual void push_back(const value_type& value)
  {
    push_back(QDateTime::currentDateTime(), value);
  }

  virtual void push_back(const QDateTime& dt, const value_type& value)
  {
    currentIdx_++;
    if (currentIdx_>=size_) currentIdx_ = 0;
    buffer_[currentIdx_] = storage_type(dt, value);
  }

  virtual storage_type& at(size_type pos)
  {
    if (pos>=size_) throw std::out_of_range("HistoryFifo index out of range.");

    size_type thePos;
    if (currentIdx_>=pos) {
      thePos = currentIdx_ - pos;
    } else {
      thePos = size_ + currentIdx_ - pos;
    }
    
    return buffer_[thePos];
  }

  virtual const storage_type& at(size_type pos) const
  {
    if (pos>=size_) throw std::out_of_range("HistoryFifo index out of range.");

    size_type thePos;
    if (currentIdx_>=pos) {
      thePos = currentIdx_ - pos;
    } else {
      thePos = size_ + currentIdx_ - pos;
    }
    
    return buffer_[thePos];
  }

  virtual QDateTime & timeAt(size_type pos)
  {
    if (pos>=size_) throw std::out_of_range("HistoryFifo index out of range.");

    size_type thePos;
    if (currentIdx_>=pos) {
      thePos = currentIdx_ - pos;
    } else {
      thePos = size_ + currentIdx_ - pos;
    }
    
    return buffer_[thePos].first;
  }

  virtual const QDateTime & timeAt(size_type pos) const
  {
    if (pos>=size_) throw std::out_of_range("HistoryFifo index out of range.");

    size_type thePos;
    if (currentIdx_>=pos) {
      thePos = currentIdx_ - pos;
    } else {
      thePos = size_ + currentIdx_ - pos;
    }
    
    return buffer_[thePos].first;
  }

  virtual value_type& valueAt(size_type pos)
  {
    if (pos>=size_) throw std::out_of_range("HistoryFifo index out of range.");

    size_type thePos;
    if (currentIdx_>=pos) {
      thePos = currentIdx_ - pos;
    } else {
      thePos = size_ + currentIdx_ - pos;
    }

    return buffer_[thePos].second;
  }

  virtual const value_type& valueAt(size_type pos) const
  {
    if (pos>=size_) throw std::out_of_range("HistoryFifo index out of range.");

    size_type thePos;
    if (currentIdx_>=pos) {
      thePos = currentIdx_ - pos;
    } else {
      thePos = size_ + currentIdx_ - pos;
    }

    return buffer_[thePos].second;
  }

  virtual const storage_type& front() const
  {
    return at(0);
  }

  virtual const storage_type& back() const
  {
    return at(size_-1);
  }

  QDateTime & timeFront()
  {
    return timeAt(0);
  }

  const QDateTime & timeFront() const
  {
    return timeAt(0);
  }

  QDateTime & timeBack() {
    return timeAt(size_-1);
  }

  const QDateTime & timeBack() const
  {
    return timeAt(size_-1);
  }
  
  virtual value_type& valueFront()
  {
    return valueAt(0);
  }

  virtual const value_type& valueFront() const
  {
    return valueAt(0);
  }

  virtual value_type& valueBack()
  {
    return valueAt(size_-1);
  }

  virtual const value_type& valueBack() const
  {
    return valueAt(size_-1);
  }

  virtual const size_type indexInPast(const qint64& seconds) const
  {
    const QDateTime & f = timeAt(0);
    for (size_type pos = 0;pos<size_;pos++) {
      const QDateTime & l = timeAt(pos);
      if (l.secsTo(f)>=seconds) return pos;
    }

    return size_-1;
  }

  qint64 deltaTime() const {
    const QDateTime & f = timeFront();
    const QDateTime & l = timeBack();
    
    return f.secsTo(l);
  }
  
  const qint64 deltaTime(int i, int j) const {
    const QDateTime & f = timeAt(i);
    const QDateTime & l = timeAt(j);
    
    return f.secsTo(l);
  }

  qint64 delta() const {
    const T & f = valueFront();
    const T & l = valueBack();
    
    return l - f;
  }

  qint64 delta(int i, int j) const {
    const T & f = valueAt(i);
    const T & l = valueAt(j);
    
    return l - f;
  }
  
  const T gradient() const {
    T d = delta();
    double dt = deltaTime();
    
    return d/dt;
  }

  const T gradient(int i, int j) const {
    T d = delta(i, j);
    double dt = deltaTime(i, j);
    
    return d/dt;
  }

  class iterator
  {
  public:

    typedef size_t size_type;

    iterator(size_type position, HistoryFifo& fifo)
      : position_(position), fifo_(fifo) { }

    iterator& operator++(int)
    {
      position_++;
      return *this;
    }

    iterator& operator++()
    {
      ++position_;
      return *this;
    }

    storage_type& operator*()
    {
      return fifo_.at(position_);
    }

    bool operator!=(const iterator& other) const
    {
      return (position_ != other.position_);
    }

  private:

    size_type position_;
    HistoryFifo<T>& fifo_;
  };

  iterator begin()
  {
    return iterator(0, *this);
  }

  iterator past(const qint64& seconds)
  {
    size_type pos = indexInPast(seconds) + 1;
    if (pos>size_) pos = size_;
    return iterator(pos, *this);
  }

  iterator end()
  {
    return iterator(size_, *this);
  }

  class const_iterator
  {
  public:

    typedef size_t size_type;

    const_iterator(size_type position, const HistoryFifo& fifo)
      : position_(position), fifo_(fifo) { }

    const_iterator& operator++(int)
    {
      position_++;
      return *this;
    }

    const_iterator& operator++()
    {
      ++position_;
      return *this;
    }

    const storage_type& operator*() const
    {
      return fifo_.at(position_);
    }

    bool operator!=(const iterator& other) const
    {
      return (position_ != other.position_);
    }

  private:

    size_type position_;
    const HistoryFifo<T>& fifo_;
  };

  const const_iterator cbegin() const
  {
    return const_iterator(0, *this);
  }

  const const_iterator cpast(const qint64& seconds) const
  {
    size_type pos = indexInPast(seconds) + 1;
    if (pos>size_) pos = size_;
    return iterator(pos, *this);
  }

  const const_iterator cend() const
  {
    return const_iterator(size_, *this);
  }

protected:

  size_type size_;
  size_type currentIdx_;
  QVector<std::pair<QDateTime,value_type> > buffer_;
};

/** @} */

#endif // HISTORYFIFO_H
