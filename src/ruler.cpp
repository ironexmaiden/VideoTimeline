#include "ruler.h"
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QPainter>
#include <QDebug> 

#define HEADER_HEIGHT 40

namespace timeline {

	Ruler::Ruler(QWidget* parent /* = Q_NULLPTR */)
		: QWidget(parent),
		mOrigin(19.0),
		mBodyBgrd(37, 38, 39),
		mHeaderBgrd(32, 32, 32),
		mInterval(30.0)
	{ 
		setMouseTracking(false); 
		resize(1000, 150);
		mIndicator = new Indicator(this);
		mIndicator->installEventFilter(this);

		mLeftBorder = new QLabel(this);
		mLeftBorder->setPixmap(QPixmap(":/images/cutleft"));
		mLeftBorder->setCursor(Qt::SizeHorCursor);
		mLeftBorder->setFixedSize(10, 86);
		mLeftBorder->move(0, HEADER_HEIGHT);
		mLeftBorder->installEventFilter(this);

		mRightBorder = new QLabel(this);
		mRightBorder->setPixmap(QPixmap(":/images/cutright"));
		mRightBorder->setFixedSize(10, 86);
		mRightBorder->move(800, HEADER_HEIGHT);
		mRightBorder->setCursor(Qt::SizeHorCursor);
		mRightBorder->installEventFilter(this);

		mRectBox = new QFrame(this);
		mRectBox->setObjectName("cutrect");
		mRectBox->setGeometry(87, 40, 700, 80);

		mContextMenu = new QMenu(this);
		mClearPoints = new QAction(tr("Clear All Points"), this);
		mCutWithCurrentPos = new QAction(tr("Cut With Currrent Position"), this);
		mMakeCurrentPoint = new QAction(tr("Mark in Current Position"), this); 
	}

	bool Ruler::eventFilter(QObject *watched, QEvent *event) {
		if (watched == mIndicator || watched == mLeftBorder 
			|| watched == mRightBorder)
		{
			static QPoint lastPnt;
			static bool isHover = false;
			if (event->type() == QEvent::MouseButtonPress) { 
				QLabel* control = dynamic_cast<QLabel*>(watched);
				QMouseEvent* e = static_cast<QMouseEvent*>(event);
				if (control->rect().contains(e->pos()) &&  
					(e->button() == Qt::LeftButton)) {
					lastPnt = e->pos();
					isHover = true;
				}
			}
			else if (event->type() == QEvent::MouseMove && isHover) {
				QMouseEvent* e = dynamic_cast<QMouseEvent*>(event);
				int dx = e->pos().x() - lastPnt.x();
				int dy = e->pos().y() - lastPnt.y();

				if (watched == mIndicator) {
					if (mIndicator->x() + dx < this->width() &&
						mIndicator->x() + dx > 0) {
						mIndicator->move(mIndicator->x() + dx, mIndicator->y());
					}
				}
				if (watched == mLeftBorder) {
					if (mLeftBorder->x() + dx <= this->width() &&
						mLeftBorder->x() + dx >= 0 &&
						mLeftBorder->x() + dx <= mRightBorder->x()) {
						mLeftBorder->move(mLeftBorder->x() + dx, mLeftBorder->y());
					}
				}
				if (watched == mRightBorder) {
					if (mRightBorder->x() + dx <= this->width() &&
						mRightBorder->x() + dx >= 0 &&
						mRightBorder->x() + dx >= mLeftBorder->x()) {
						mRightBorder->move(mRightBorder->x() + dx, mRightBorder->y());
					}
				}
			}
			else if (event->type() == QEvent::MouseButtonRelease && isHover)
			{
				isHover = false;
			}
		}

		return false;
	} 

	void Ruler::contextMenuEvent(QContextMenuEvent *event) {
		mContextMenu->addAction(mClearPoints);
		mContextMenu->addAction(mCutWithCurrentPos);
		mContextMenu->addAction(mMakeCurrentPoint);
		mContextMenu->exec(QCursor::pos());
		event->accept();
	} 

	void Ruler::mouseMoveEvent(QMouseEvent *event) {
		mCursorPos = event->pos();
		update();
		QWidget::mouseMoveEvent(event);
	} 

	void Ruler::wheelEvent(QWheelEvent *event) {
		QPoint numDegrees = event->angleDelta() / 8; 
		if (!numDegrees.isNull()) {
			if (numDegrees.y() > 0) {
				onZoomerIn();
			}
			if (numDegrees.y() < 0)
			{
				onZoomerOut();
			}
		}
		event->accept();
	}

	void Ruler::setOrigin(const qreal origin)
	{
		if (mOrigin != origin) {
			mOrigin = origin;
			update();
		}
	}  
	 
	void Ruler::paintEvent(QPaintEvent *event) { 
		QPainter painter(this);
		QFont font = painter.font();
		font.setPointSize(10);
		painter.setFont(font);
		painter.setRenderHints(QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing);
		QPen pen(QColor(181, 181, 181), 1);
		painter.setPen(pen); 

		QRectF rulerRect = this->rect(); 
		// paint header background color
		painter.fillRect(QRect(rulerRect.left(), rulerRect.top(), 
			rulerRect.width(), HEADER_HEIGHT), mHeaderBgrd);
		// paint body background color
		painter.fillRect(QRect(rulerRect.left(), rulerRect.top() + HEADER_HEIGHT, 
			rulerRect.width(), rulerRect.height() - HEADER_HEIGHT), mBodyBgrd);

		if (!mDuration.isValid()) {
			//return;
		}

		// draw tickers and time labels
		drawScaleRuler(&painter, rulerRect);  
	}
	
	QString Ruler::getTickerString(qreal tickerNo) {
		
		return "";
	}

	void Ruler::onZoomerIn() {
		if (mInterval > 20) {
			mInterval -= 2;
			resize(this->width() - 50, this->height());
		} 
	}

	void Ruler::onZoomerOut() {
		if (mInterval < 40) {
			mInterval += 2;
			resize(this->width() + 50, this->height());
		} 
	}

	void Ruler::drawScaleRuler(QPainter* painter, QRectF rulerRect) { 
		qreal rulerStartMark = rulerRect.left();
		qreal rulerEndMark = rulerRect.right();

		if (mOrigin >= rulerStartMark && mOrigin <= rulerEndMark) {
			drawTickers(painter, rulerRect, mOrigin, rulerEndMark); 
		}
	}

	void Ruler::drawTickers(QPainter* painter, QRectF rulerRect, qreal startMark, qreal endMark)
	{
		for (qreal current = startMark; current <= endMark; current += mInterval)
		{
			qreal x1 = current;
			qreal y1 = rulerRect.top() + HEADER_HEIGHT - 5;
			qreal x2 = current;
			qreal y2 = rulerRect.bottom() - HEADER_HEIGHT;
			painter->drawLine(QLineF(x1, y1, x2, y2));
			//painter->drawText(x1 - 10, y1 - HEADER_HEIGHT/4, getTickerString(current));
		}
	}
}


