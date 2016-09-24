/*From version of May 16 2010
	ORIGINAL COPYRIGHT HEADER
	PictureFlow - animated image show widget
	http://pictureflow.googlecode.com

	Copyright (C) 2008 Ariya Hidayat (ariya@kde.org)
	Copyright (C) 2007 Ariya Hidayat (ariya@kde.org)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#ifndef PICTUREFLOW_H
#define PICTUREFLOW_H

#include <QWidget>

class PictureFlowPrivate;

	/// \class PictureFlow
	/// \brief The PictureFlow class
	///Class PictureFlow implements an image show widget with animation effect
	///like Apple's CoverFlow (in iTunes and iPod). Images are arranged in form
	///of slides, one main slide is shown at the center with few slides on
	///the left and right sides of the center slide. When the next or previous
	///slide is brought to the front, the whole slides flow to the right or
	///the right with smooth animation effect; until the new slide is finally
	///placed at the center.
class PictureFlow : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
	Q_PROPERTY(QSize slideSize READ slideSize)
	Q_PROPERTY(float slideSizeRatio READ slideSizeRatio WRITE setSlideSizeRatio)
	Q_PROPERTY(int slideCount READ slideCount)
	Q_PROPERTY(int centerIndex READ centerIndex WRITE setCenterIndex)

public:

	enum ReflectionEffect
	{
		NoReflection,
		PlainReflection,
		BlurredReflection
	};

	///
	/// \brief PictureFlow
	/// \param parent
	///Creates a new PictureFlow widget.
	PictureFlow(QWidget* parent = 0);

	///
	/// \brief ~PictureFlow
	///Destroys the widget.
	~PictureFlow();

	///
	/// \brief backgroundColor
	/// \return QColor
	///Returns the background color.
	QColor backgroundColor() const;

	///
	/// \brief setBackgroundColor
	/// \param c
	///Sets the background color. By default it is black.
	void setBackgroundColor(const QColor& c);

	/*!
		Returns the dimension of each slide (in pixels).
	*/
	///
	/// \brief slideSize
	/// \return QSize
	///Returns the dimension of each slide (in pixels).
	QSize slideSize() const;

	///
	/// \brief slideSizeRatio
	/// \return float
	///Returns the ratio dimension of each slide (Height/Width).
	float slideSizeRatio() const;

	///
	/// \brief setSlideSizeRatio
	/// \param ratio
	///Sets the ratio of dimension of each slide (in pixels).
	void setSlideSizeRatio(float ratio);

	///
	/// \brief slideCount
	/// \return int
	///Returns the total number of slides.
	int slideCount() const;

	///
	/// \brief slide
	/// \param index
	/// \return QImage
	///Returns QImage of specified slide.
	QImage slide(int index) const;

	///
	/// \brief slideAt
	/// \param point
	/// \return int
	///Returns QImage of specified slide.
	int slideAt(QPoint point) const;

	///
	/// \brief centerIndex
	/// \return int
	///Returns the index of slide currently shown in the middle of the viewport.
	int centerIndex() const;

	///
	/// \brief reflectionEffect
	/// \return ReflectionEffect
	///Returns the effect applied to the reflection.
	ReflectionEffect reflectionEffect() const;

	///
	/// \brief setReflectionEffect
	/// \param effect
	///Sets the effect applied to the reflection. The default is PlainReflection.
	void setReflectionEffect(ReflectionEffect effect);

public slots:

	///
	/// \brief addSlide
	/// \param image QImage of slide
	///Adds a new slide.
	void addSlide(const QImage& image);

	///
	/// \brief addSlide
	/// \param pixmap QPixmap of slide
	///Adds a new slide.
	void addSlide(const QPixmap& pixmap);

	///
	/// \brief setSlide
	/// \param index index of slide
	/// \param image QImage of slide
	///Sets an image for specified slide. If the slide already exists,
	///it will be replaced.

	void setSlide(int index, const QImage& image);

	///
	/// \brief setSlide
	/// \param index index of slide
	/// \param pixmap QPixmap of slide
	///Sets a pixmap for specified slide. If the slide already exists,
	///it will be replaced.
	void setSlide(int index, const QPixmap& pixmap);

	///
	/// \brief setCenterIndex
	/// \param index Index of slide to center
	///Sets slide to be shown in the middle of the viewport. No animation
	///effect will be produced, unlike using showSlide.
	void setCenterIndex(int index);

	///
	/// \brief clear
	///Clears all slides.
	void clear();

	///
	/// \brief showPrevious
	///Shows previous slide using animation effect.
	void showPrevious();

	///
	/// \brief showNext
	///Shows next slide using animation effect.
	void showNext();

	///
	/// \brief showSlide
	/// \param index
	///Go to specified slide using animation effect.
	void showSlide(int index);

	///
	/// \brief render
	///Rerender the widget. Normally this function will be automatically invoked
	///whenever necessary, e.g. during the transition animation.
	void render();

	///
	/// \brief triggerRender
	///Schedules a rendering update. Unlike render(), this function does not cause
	///immediate rendering.
	void triggerRender();

signals:
	///
	/// \brief centerIndexChanged
	/// \param index
	///Signals when index was changed.
	void centerIndexChanged(int index);
	///
	/// \brief mouseMoveOverSlideEvent
	/// \param event
	/// \param slideIndex
	///Signals when mouse move over a slide.
	void mouseMoveOverSlideEvent(QMouseEvent* event, int slideIndex);
	///
	/// \brief dragEnterEventOccurs
	/// \param event
	///Signals when a drag enters on a slide.
	void dragEnterEventOccurs(QDragEnterEvent *event);
	///
	/// \brief dragMoveEventOccurs
	/// \param event
	///Signals when a drag move over a slide.
	void dragMoveEventOccurs(QDragMoveEvent *event);
	///
	/// \brief dropEventOccurs
	/// \param event
	///Signals when something is dropped on slide.
	void dropEventOccurs(QDropEvent *event);

protected:
	void paintEvent(QPaintEvent* event);
	void keyPressEvent(QKeyEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void resizeEvent(QResizeEvent* event);
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);

private slots:
	void updateAnimation();

private:
	PictureFlowPrivate* d;
	QPoint dragMoveLastPos;
};

#endif // PICTUREFLOW_H

