// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2025
#include "image_view.h"

#include "ayu/features/message_shot/message_shot.h"
#include "styles/style_ayu_styles.h"

#include "ayu/utils/telegram_helpers.h"
#include "styles/style_chat.h"
#include "ui/painter.h"

ImageView::ImageView(QWidget *parent)
	: RpWidget(parent) {
}

void ImageView::setImage(const QImage &image) {
	if (this->image == image) {
		return;
	}

	const auto set = [=]
	{
		this->prevImage = this->image;
		this->image = image;

		const auto size = image.size() / style::DevicePixelRatio();
		setMinimumSize(size.grownBy(st::imageViewInnerPadding));

		if (this->animation.animating()) {
			this->animation.stop();
		}

		if (this->prevImage.isNull()) {
			update();
			return;
		}

		this->animation.start(
			[=]
			{
				update();
			},
			0.0,
			1.0,
			300,
			anim::easeInCubic);
	};

	if (this->image.isNull()) {
		set();
		return;
	}

	dispatchToMainThread(set, 100);
}

void ImageView::setPreviewBackgroundVisible(bool visible) {
	if (previewBackgroundVisible == visible) {
		return;
	}
	previewBackgroundVisible = visible;
	update();
}

QImage ImageView::getImage() const {
	return image;
}

void ImageView::paintEvent(QPaintEvent *e) {
	Painter p(this);

	QPainterPath path;
	path.addRoundedRect(rect(), st::roundRadiusLarge, st::roundRadiusLarge);

	if (previewBackgroundVisible) {
		p.fillPath(path, AyuFeatures::MessageShot::makeDefaultBackgroundColor());
	} else {
		// Transparency preview for screenshots without an explicit background.
		const auto check = 12;
		const auto light = st::boxBg->c.lighter(115);
		const auto dark = st::boxBg->c.darker(110);

		p.save();
		p.setClipPath(path);
		p.fillRect(rect(), light);
		for (auto y = 0; y < height(); y += check) {
			for (auto x = 0; x < width(); x += check) {
				if (((x / check) + (y / check)) % 2 == 0) {
					p.fillRect(QRect(x, y, check, check), dark);
				}
			}
		}
		p.restore();
	}

	if (!prevImage.isNull()) {
		const auto realRect = rect().marginsRemoved(st::imageViewInnerPadding);

		const auto resizedRect = QRect(
			(realRect.width() - prevImage.width() / style::DevicePixelRatio()) / 2 + st::imageViewInnerPadding.left(),
			(realRect.height() - prevImage.height() / style::DevicePixelRatio()) / 2 + st::imageViewInnerPadding.top(),
			prevImage.width() / style::DevicePixelRatio(),
			prevImage.height() / style::DevicePixelRatio());

		const auto opacity = 1.0 - animation.value(1.0);
		p.setOpacity(opacity);
		p.drawImage(resizedRect, prevImage);
		p.setOpacity(1.0);
	}

	if (!image.isNull()) {
		const auto realRect = rect().marginsRemoved(st::imageViewInnerPadding);

		const auto resizedRect = QRect(
			(realRect.width() - image.width() / style::DevicePixelRatio()) / 2 + st::imageViewInnerPadding.left(),
			(realRect.height() - image.height() / style::DevicePixelRatio()) / 2 + st::imageViewInnerPadding.top(),
			image.width() / style::DevicePixelRatio(),
			image.height() / style::DevicePixelRatio());

		const auto opacity = animation.value(1.0);
		p.setOpacity(opacity);
		p.drawImage(resizedRect, image);
		p.setOpacity(1.0);
	}
}

void ImageView::mousePressEvent(QMouseEvent *e) {
}
