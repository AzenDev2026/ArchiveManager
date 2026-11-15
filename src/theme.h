#pragma once

#include <QIcon>
#include <QPalette>
#include <QString>

namespace Theme {

enum class Mode { Dark, Light };

Mode loadMode();
void saveMode(Mode mode);

QPalette palette(Mode mode);
QString styleSheet(Mode mode);

// Returns a themed icon, falling back to a secondary name, then to an empty icon.
QIcon icon(const QString &primary, const QString &fallback = QString());

} // namespace Theme
