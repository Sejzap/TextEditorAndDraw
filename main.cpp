// ============================================================
//  记事本 + 画图 合并程序 (C++/Qt6)
// ============================================================
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QFileDialog>
#include <QIcon>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QColorDialog>
#include <QInputDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSlider>
#include <QCheckBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QGroupBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QFont>
#include <QImage>
#include <QVector>
#include <QKeySequence>
#include <QShortcut>
#include <QTimer>
#include <QHash>
#include <QStringList>
#include <QStatusBar>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <shobjidl.h>       
#  pragma comment(lib, "shell32.lib") 
#endif

// ============================================================
//  作者链接
// ============================================================
static const QString AUTHOR_URL =
"https://space.bilibili.com/3706964049660691?spm_id_from=333.1007.0.0";

// ============================================================
//  编码转换
// ============================================================
static QByteArray encodeText(const QString& text, const QString& encName, QString& error) {
    QString enc = encName.trimmed().toLower();

    if (enc == "utf-8" || enc == "utf8")            return text.toUtf8();
    if (enc == "utf-8 bom" || enc == "utf-8-sig")   return QByteArray("\xEF\xBB\xBF") + text.toUtf8();

    if (enc == "ascii") {
        QByteArray out;
        for (QChar c : text) {
            if (c.unicode() > 0x7F) {
                error = QString("字符 '%1' 无法用 ASCII 编码").arg(c);
                return {};
            }
            out.append(static_cast<char>(c.unicode()));
        }
        return out;
    }
    if (enc == "latin-1" || enc == "iso-8859-1") {
        QByteArray out;
        for (QChar c : text) {
            if (c.unicode() > 0xFF) {
                error = QString("字符 '%1' 无法用 Latin-1 编码").arg(c);
                return {};
            }
            out.append(static_cast<char>(c.unicode()));
        }
        return out;
    }

#ifdef Q_OS_WIN
    int cp = -1;
    if (enc == "gbk" || enc == "gb2312") cp = 936;
    else if (enc == "gb18030")                cp = 54936;
    else if (enc == "big5")                   cp = 950;
    else { error = "不认识的编码名：" + encName; return {}; }

    int len = WideCharToMultiByte(cp, 0,
        reinterpret_cast<const wchar_t*>(text.utf16()), text.length(),
        nullptr, 0, nullptr, nullptr);
    if (len <= 0) { error = "编码转换失败（含非法字符）"; return {}; }
    QByteArray out(len, '\0');
    WideCharToMultiByte(cp, 0,
        reinterpret_cast<const wchar_t*>(text.utf16()), text.length(),
        out.data(), len, nullptr, nullptr);
    return out;
#else
    error = "此平台不支持 " + encName + " 编码";
    return {};
#endif
}

static QString decodeText(const QByteArray& data, const QString& encName) {
    QString enc = encName.trimmed().toLower();
    if (enc == "utf-8" || enc == "utf8") return QString::fromUtf8(data);
    if (enc == "latin-1" || enc == "iso-8859-1" || enc == "ascii") return QString::fromLatin1(data);

#ifdef Q_OS_WIN
    int cp = -1;
    if (enc == "gbk" || enc == "gb2312") cp = 936;
    else if (enc == "gb18030")                cp = 54936;
    else if (enc == "big5")                   cp = 950;

    if (cp > 0) {
        int len = MultiByteToWideChar(cp, 0, data.constData(), data.size(), nullptr, 0);
        if (len <= 0) return {};
        QVector<wchar_t> buf(len);
        MultiByteToWideChar(cp, 0, data.constData(), data.size(), buf.data(), len);
        return QString::fromWCharArray(buf.data(), len);
    }
#endif
    return QString::fromUtf8(data);
}

// ============================================================
//  自定义保存对话框
// ============================================================
class SaveDialog : public QDialog {
    
public:
    explicit SaveDialog(QWidget* parent,
        const QString& initialFile = "untitled.txt",
        const QString& initialDir = QString())
        : QDialog(parent)
    {
        setWindowTitle("保存");
        setModal(true);
        setMinimumWidth(620);

        auto* rootLay = new QVBoxLayout(this);

        auto* row1 = new QHBoxLayout();
        row1->addWidget(new QLabel("文件名："));
        QString base = QFileInfo(initialFile).completeBaseName();
        QString dir = initialDir.isEmpty() ? QDir::currentPath() : initialDir;
        m_pathEdit = new QLineEdit(QDir(dir).filePath(base));
        row1->addWidget(m_pathEdit, 1);
        auto* browseBtn = new QPushButton("浏览…");
        browseBtn->setFixedWidth(80);
        row1->addWidget(browseBtn);
        rootLay->addLayout(row1);

        auto* row2 = new QHBoxLayout();
        row2->addWidget(new QLabel("保存类型："));
        m_typeCombo = new QComboBox();
        m_typeCombo->addItems({ "TXT","PY","HTML","JS","MD","BAT","CMD","MD5","所有格式" });
        m_typeCombo->setFixedWidth(130);
        row2->addWidget(m_typeCombo);
        row2->addStretch();
        rootLay->addLayout(row2);

        auto* row3 = new QHBoxLayout();
        row3->addWidget(new QLabel("编码："));
        m_encCombo = new QComboBox();
        m_encCombo->setEditable(true);
        m_encCombo->addItems({ "utf-8","UTF-8 BOM","gbk","gb18030","gb2312","big5","ascii","latin-1" });
        m_encCombo->setFixedWidth(160);
        row3->addWidget(m_encCombo);
        auto* hint = new QLabel("UTF-8 BOM = 带 BOM（记事本/Excel 友好）");
        hint->setStyleSheet("color:#888;");
        row3->addWidget(hint);
        row3->addStretch();
        rootLay->addLayout(row3);

        m_previewLabel = new QLabel();
        m_previewLabel->setStyleSheet("color:#0a7;");
        rootLay->addWidget(m_previewLabel);

        auto* btnRow = new QHBoxLayout();
        btnRow->addStretch();
        auto* cancelBtn = new QPushButton("取消");
        auto* okBtn = new QPushButton("保存");
        okBtn->setDefault(true);
        cancelBtn->setFixedWidth(90);
        okBtn->setFixedWidth(90);
        btnRow->addWidget(cancelBtn);
        btnRow->addWidget(okBtn);
        rootLay->addLayout(btnRow);

        connect(browseBtn, &QPushButton::clicked, this, &SaveDialog::browse);
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        connect(okBtn, &QPushButton::clicked, this, &SaveDialog::onOk);
        connect(m_pathEdit, &QLineEdit::textChanged, this, &SaveDialog::updatePreview);
        connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SaveDialog::updatePreview);
        connect(m_pathEdit, &QLineEdit::textEdited, this, &SaveDialog::syncTypeFromPath);

        QString initExt = QFileInfo(initialFile).suffix().toLower();
        if (!initExt.isEmpty()) {
            QString up = initExt.toUpper();
            int idx = m_typeCombo->findText(up);
            if (idx >= 0) m_typeCombo->setCurrentIndex(idx);
        }

        updatePreview();
        m_pathEdit->setFocus();
        m_pathEdit->setCursorPosition(m_pathEdit->text().length());
    }

    QString finalPath() const { return m_finalPath; }
    QString encoding()  const { return m_encCombo->currentText().trimmed(); }

private:
    void browse() {
        QString current = m_pathEdit->text().trimmed();
        QString ext = extForType(m_typeCombo->currentText());

        QString dir, name;
        if (!current.isEmpty()) {
            QFileInfo fi(current);
            dir = fi.absolutePath();
            name = fi.completeBaseName();
        }
        else {
            dir = QDir::currentPath();
            name = "untitled";
        }

        QString filter = "所有文件 (*.*)";
        if (!ext.isEmpty())
            filter = QString("%1 文件 (*%1);;所有文件 (*.*)").arg(ext);

        QString p = QFileDialog::getSaveFileName(this, "选择保存位置",
            QDir(dir).filePath(name + ext), filter);
        if (p.isEmpty()) return;

        QFileInfo fi(p);
        m_pathEdit->setText(QDir(fi.absolutePath()).filePath(fi.completeBaseName()));

        QString newExt = fi.suffix().toLower();
        if (!newExt.isEmpty()) {
            int idx = m_typeCombo->findText(newExt.toUpper());
            if (idx >= 0) m_typeCombo->setCurrentIndex(idx);
            else          m_typeCombo->setCurrentText("所有格式");
        }
    }

    void updatePreview() {
        m_previewLabel->setText("将保存为：" + computeFinalPath());
    }

    void syncTypeFromPath(const QString& text) {
        QString ext = QFileInfo(text.trimmed()).suffix().toLower();
        if (ext.isEmpty()) return;
        QString up = "." + ext;
        for (auto& known : KNOWN_EXTS) {
            if (known == up) {
                int idx = m_typeCombo->findText(ext.toUpper());
                if (idx >= 0) m_typeCombo->setCurrentIndex(idx);
                return;
            }
        }
        m_typeCombo->setCurrentText("所有格式");
    }

    void onOk() {
        if (m_pathEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "提示", "请输入文件名");
            return;
        }
        if (m_encCombo->currentText().trimmed().isEmpty()) {
            QMessageBox::warning(this, "提示", "请选择编码");
            return;
        }
        m_finalPath = computeFinalPath();
        accept();
    }

private:
    static QString extForType(const QString& type) {
        static const QHash<QString, QString> map = {
            {"TXT",".txt"},{"PY",".py"},{"HTML",".html"},{"JS",".js"},
            {"MD",".md"},{"BAT",".bat"},{"CMD",".cmd"},{"MD5",".md5"},
            {"所有格式",""}
        };
        return map.value(type, "");
    }

    static const QStringList KNOWN_EXTS;

    QString computeFinalPath() const {
        QString base = m_pathEdit->text().trimmed();
        QString ext = extForType(m_typeCombo->currentText());
        if (ext.isEmpty()) return base;

        QString curExt = QFileInfo(base).suffix();
        if (!curExt.isEmpty() && KNOWN_EXTS.contains("." + curExt.toLower()))
            base = base.left(base.length() - curExt.length() - 1);

        return base + ext;
    }

    QLineEdit* m_pathEdit;
    QComboBox* m_typeCombo;
    QComboBox* m_encCombo;
    QLabel* m_previewLabel;
    QString      m_finalPath;
};

const QStringList SaveDialog::KNOWN_EXTS = {
    ".txt", ".py", ".html", ".js", ".md", ".bat", ".cmd", ".md5"
};

// ============================================================
//  画图区
// ============================================================
class CanvasWidget : public QWidget {
    
public:
    enum Tool { Pencil, Line, Rect, Oval, Eraser, Text };

    explicit CanvasWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_StaticContents);
        setMouseTracking(true);
        setCursor(Qt::CrossCursor);
        setFocusPolicy(Qt::StrongFocus);
        m_undoStack.append(QVector<Shape>());
    }

    void setTool(Tool t) { m_tool = t; setCursor(t == Text ? Qt::IBeamCursor : Qt::CrossCursor); }
    void setFgColor(const QColor& c) { m_fg = c; }
    void setBgColor(const QColor& c) { m_bg = c; }
    void setBrushSize(int s) { m_brushSize = qMax(1, s); }
    void setFillEnabled(bool f) { m_fill = f; }
    QColor fgColor() const { return m_fg; }
    QColor bgColor() const { return m_bg; }

    void undo() {
        if (m_undoStack.size() <= 1) return;
        m_redoStack.append(m_undoStack.takeLast());
        restoreState(m_undoStack.last());
    }
    void redo() {
        if (m_redoStack.isEmpty()) return;
        auto s = m_redoStack.takeLast();
        m_undoStack.append(s);
        restoreState(s);
    }
    void clearCanvas() {
        m_shapes.clear();
        pushUndo();
        update();
    }

    QImage renderToImage() const {
        QImage img(size(), QImage::Format_ARGB32);
        img.fill(m_bg);
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing, true);
        for (const auto& s : m_shapes) drawShape(p, s);
        return img;
    }

    QSize sizeHint() const override { return { 900, 600 }; }



protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        for (const auto& s : m_shapes) drawShape(p, s);
        if (m_drawing) drawShape(p, m_current);
    }

    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() != Qt::LeftButton) return;
        m_startPos = e->pos();

        if (m_tool == Text) {
            
            bool ok = false;
            QString s = QInputDialog::getText(this, "输入文字", "请输入要添加的文字：",
                QLineEdit::Normal, QString(), &ok);
            if (ok) addTextAt(m_startPos, s);
            return;
        }

        m_current = Shape();
        m_current.tool = m_tool;
        m_current.fg = (m_tool == Eraser) ? m_bg : m_fg;
        m_current.fill = m_bg;
        m_current.width = (m_tool == Eraser) ? qMax(8, m_brushSize * 3) : m_brushSize;
        m_current.filled = m_fill;
        m_current.points.append(m_startPos);

        m_drawing = true;
    }

    void mouseMoveEvent(QMouseEvent* e) override {
        if (!m_drawing) return;
        QPoint pos = e->pos();

        if (m_tool == Pencil || m_tool == Eraser) {
            m_current.points.append(pos);
        }
        else {
            if (e->modifiers() & Qt::ShiftModifier) pos = constrain(m_startPos, pos);
            if (m_current.points.size() < 2) m_current.points.append(pos);
            else m_current.points[1] = pos;
        }
        update();
    }

    void mouseReleaseEvent(QMouseEvent* e) override {
        Q_UNUSED(e);
        if (!m_drawing) return;

        bool discard = false;
        if (m_tool == Line || m_tool == Rect || m_tool == Oval) {
            if (m_current.points.size() < 2) discard = true;
            else if (m_current.points[0] == m_current.points[1]) discard = true;
        }
        if (!discard) {
            m_shapes.append(m_current);
            pushUndo();
        }
        m_drawing = false;
        m_current = Shape();
        update();
    }

public:
    void addTextAt(const QPoint& pos, const QString& text) {
        if (text.isEmpty()) return;
        Shape s;
        s.tool = Text;
        s.points.append(pos);
        s.fg = m_fg;
        s.width = m_brushSize;
        s.text = text;
        QFont f("Microsoft YaHei");
        f.setPixelSize(qMax(12, m_brushSize * 4));
        s.font = f;
        m_shapes.append(s);
        pushUndo();
        update();
    }

private:
    struct Shape {
        Tool             tool = Pencil;
        QVector<QPoint>  points;
        QColor           fg = Qt::black;
        QColor           fill = Qt::white;
        int              width = 1;
        bool             filled = false;
        QString          text;
        QFont            font;
    };

    static const int MAX_UNDO = 60;

    Tool     m_tool = Pencil;
    QColor   m_fg = Qt::black;
    QColor   m_bg = Qt::white;
    int      m_brushSize = 3;
    bool     m_fill = false;

    QVector<Shape>    m_shapes;
    Shape             m_current;
    bool              m_drawing = false;
    QPoint            m_startPos;

    QVector<QVector<Shape>> m_undoStack;
    QVector<QVector<Shape>> m_redoStack;

    void pushUndo() {
        m_undoStack.append(m_shapes);
        while (m_undoStack.size() > MAX_UNDO) m_undoStack.removeFirst();
        m_redoStack.clear();
    }
    void restoreState(const QVector<Shape>& s) {
        m_shapes = s;
        update();
    }

    static QPoint constrain(const QPoint& a, const QPoint& b) {
        int dx = b.x() - a.x();
        int dy = b.y() - a.y();
        int adx = qAbs(dx), ady = qAbs(dy);
        if (adx > 2 * ady) return { b.x(), a.y() };
        if (ady > 2 * adx) return { a.x(), b.y() };
        int s = qMin(adx, ady);
        return { a.x() + s * (dx >= 0 ? 1 : -1),
                 a.y() + s * (dy >= 0 ? 1 : -1) };
    }

    void drawShape(QPainter& p, const Shape& s) const {
        QPen pen(s.fg, s.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);

        switch (s.tool) {
        case Pencil:
        case Eraser: {
            if (s.points.size() < 2) return;
            QPainterPath path(s.points[0]);
            for (int i = 1; i < s.points.size(); ++i) path.lineTo(s.points[i]);
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);
            break;
        }
        case Line:
            if (s.points.size() < 2) return;
            p.drawLine(s.points[0], s.points[1]);
            break;
        case Rect: {
            if (s.points.size() < 2) return;
            QRect r = QRect(s.points[0], s.points[1]).normalized();
            if (s.filled) p.fillRect(r, s.fill);
            p.setBrush(Qt::NoBrush);
            p.drawRect(r);
            break;
        }
        case Oval: {
            if (s.points.size() < 2) return;
            QRect r = QRect(s.points[0], s.points[1]).normalized();
            p.setBrush(s.filled ? QBrush(s.fill) : Qt::NoBrush);
            p.drawEllipse(r);
            break;
        }
        case Text:
            if (s.points.isEmpty() || s.text.isEmpty()) return;
            p.setFont(s.font);
            p.drawText(s.points[0], s.text);
            break;
        }
    }
};

// ============================================================
//  主窗口
// ============================================================
class MainWindow : public QMainWindow {
    
public:
    MainWindow() {
        setWindowTitle("记事本 + 画图");
        resize(1000, 700);

        m_tabs = new QTabWidget(this);
        setCentralWidget(m_tabs);

        m_textEdit = new QTextEdit();
        m_textEdit->setFont(QFont("Consolas", 11));
        m_textEdit->setAcceptRichText(false);
        m_tabs->addTab(m_textEdit, "记事本");

        m_canvas = new CanvasWidget();
        auto* paintPage = new QWidget();
        auto* play = new QVBoxLayout(paintPage);
        play->setContentsMargins(4, 4, 4, 4);
        play->setSpacing(4);

        auto* bar1 = new QHBoxLayout();
        auto* group = new QButtonGroup(this);
        group->setExclusive(true);
        struct { const char* name; CanvasWidget::Tool tool; } tools[] = {
            {"铅笔", CanvasWidget::Pencil}, {"直线", CanvasWidget::Line},
            {"矩形", CanvasWidget::Rect},   {"椭圆", CanvasWidget::Oval},
            {"橡皮", CanvasWidget::Eraser}, {"文字", CanvasWidget::Text},
        };
        for (auto& t : tools) {
            auto* rb = new QRadioButton(t.name);
            rb->setObjectName("toolbtn");
            rb->setProperty("tool", static_cast<int>(t.tool));
            group->addButton(rb, static_cast<int>(t.tool));
            bar1->addWidget(rb);
        }
        group->button(CanvasWidget::Pencil)->setChecked(true);
        connect(group, QOverload<int>::of(&QButtonGroup::idClicked),
            this, &MainWindow::onToolChanged);

        bar1->addSpacing(16);
        bar1->addWidget(new QLabel("粗细:"));
        auto* sizeSlider = new QSlider(Qt::Horizontal);
        sizeSlider->setRange(1, 40);
        sizeSlider->setValue(3);
        sizeSlider->setFixedWidth(140);
        bar1->addWidget(sizeSlider);
        connect(sizeSlider, &QSlider::valueChanged, m_canvas, &CanvasWidget::setBrushSize);

        auto* fillChk = new QCheckBox("填充图形");
        bar1->addSpacing(12);
        bar1->addWidget(fillChk);
        connect(fillChk, &QCheckBox::toggled, m_canvas, &CanvasWidget::setFillEnabled);
        bar1->addStretch();
        play->addLayout(bar1);

        auto* bar2 = new QHBoxLayout();
        bar2->addWidget(new QLabel("画笔颜色:"));
        auto* fgBtn = new QPushButton();
        fgBtn->setFixedSize(36, 24);
        fgBtn->setStyleSheet("background:#000000; border:1px solid #666;");
        bar2->addWidget(fgBtn);
        connect(fgBtn, &QPushButton::clicked, this, &MainWindow::chooseFgColor);

        bar2->addSpacing(12);
        bar2->addWidget(new QLabel("填充颜色:"));
        auto* bgBtn = new QPushButton();
        bgBtn->setFixedSize(36, 24);
        bgBtn->setStyleSheet("background:#ffffff; border:1px solid #666;");
        bar2->addWidget(bgBtn);
        connect(bgBtn, &QPushButton::clicked, this, &MainWindow::chooseBgColor);

        bar2->addSpacing(12);
        const QStringList quick = { "#000000","#ff0000","#00a000","#0064ff","#ffd200","#ffffff" };
        for (auto& c : quick) {
            auto* b = new QPushButton();
            b->setFixedSize(24, 24);
            b->setStyleSheet(QString("background:%1; border:1px solid #888;").arg(c));
            bar2->addWidget(b);
            connect(b, &QPushButton::clicked, this, [this, fgBtn, c] {
                m_canvas->setFgColor(QColor(c));
                fgBtn->setStyleSheet(QString("background:%1; border:1px solid #666;").arg(c));
                });
        }

        bar2->addSpacing(20);
        auto* undoBtn = new QPushButton("撤销");
        auto* redoBtn = new QPushButton("恢复");
        auto* clearBtn = new QPushButton("清空");
        auto* saveImgBtn = new QPushButton("保存图片");
        bar2->addWidget(undoBtn);
        bar2->addWidget(redoBtn);
        bar2->addWidget(clearBtn);
        bar2->addWidget(saveImgBtn);
        bar2->addStretch();
        play->addLayout(bar2);

        connect(undoBtn, &QPushButton::clicked, m_canvas, &CanvasWidget::undo);
        connect(redoBtn, &QPushButton::clicked, m_canvas, &CanvasWidget::redo);
        connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearCanvas);
        connect(saveImgBtn, &QPushButton::clicked, this, &MainWindow::saveImage);


        play->addWidget(m_canvas, 1);
        m_tabs->addTab(paintPage, "画图");

        buildMenus();

        auto* scOpen = new QShortcut(QKeySequence("Ctrl+O"), this);
        connect(scOpen, &QShortcut::activated, this, &MainWindow::openFile);
        auto* scSave = new QShortcut(QKeySequence("Ctrl+S"), this);
        connect(scSave, &QShortcut::activated, this, [this] {
            if (m_tabs->currentIndex() == 0) saveTextFile();
            else                              saveImage();
            });
        auto* scF1 = new QShortcut(QKeySequence("F1"), this);
        connect(scF1, &QShortcut::activated, this, [] {
            QDesktopServices::openUrl(QUrl(AUTHOR_URL));
            });

        auto* scUndo = new QShortcut(QKeySequence("Ctrl+Z"), this);
        scUndo->setContext(Qt::WidgetWithChildrenShortcut);
        connect(scUndo, &QShortcut::activated, this, [this] {
            if (m_tabs->currentIndex() == 1) m_canvas->undo();
            });
        auto* scRedo = new QShortcut(QKeySequence("Ctrl+Y"), this);
        scRedo->setContext(Qt::WidgetWithChildrenShortcut);
        connect(scRedo, &QShortcut::activated, this, [this] {
            if (m_tabs->currentIndex() == 1) m_canvas->redo();
            });

        statusBar()->showMessage("就绪");
    }

private:
    void buildMenus() {
        auto* menubar = menuBar();

        auto* fileMenu = menubar->addMenu("文件");
        fileMenu->addAction("打开…", QKeySequence("Ctrl+O"), this, &MainWindow::openFile);
        fileMenu->addAction("保存…", QKeySequence("Ctrl+S"), this, &MainWindow::saveTextFile);
        fileMenu->addSeparator();
        fileMenu->addAction("保存图片…", this, &MainWindow::saveImage);
        fileMenu->addSeparator();
        fileMenu->addAction("退出", this, &QWidget::close);

        auto* editMenu = menubar->addMenu("编辑");
        editMenu->addAction("撤销", QKeySequence("Ctrl+Z"), this, [this] {
            if (m_tabs->currentIndex() == 0) m_textEdit->undo();
            else                              m_canvas->undo();
            });
        editMenu->addAction("重做", QKeySequence("Ctrl+Y"), this, [this] {
            if (m_tabs->currentIndex() == 0) m_textEdit->redo();
            else                              m_canvas->redo();
            });
        editMenu->addSeparator();
        editMenu->addAction("剪切", QKeySequence("Ctrl+X"), this, [this] {
            if (m_tabs->currentIndex() == 0) m_textEdit->cut();
            });
        editMenu->addAction("复制", QKeySequence("Ctrl+C"), this, [this] {
            if (m_tabs->currentIndex() == 0) m_textEdit->copy();
            });
        editMenu->addAction("粘贴", QKeySequence("Ctrl+V"), this, [this] {
            if (m_tabs->currentIndex() == 0) m_textEdit->paste();
            });
        editMenu->addSeparator();
        editMenu->addAction("全选", QKeySequence("Ctrl+A"), this, [this] {
            if (m_tabs->currentIndex() == 0) m_textEdit->selectAll();
            });

        auto* paintMenu = menubar->addMenu("画图");
        paintMenu->addAction("清空画布", this, &MainWindow::onClearCanvas);
        paintMenu->addAction("保存图片…", this, &MainWindow::saveImage);

        auto* helpMenu = menubar->addMenu("帮助");
        helpMenu->addAction("作者", this, [] {
            QDesktopServices::openUrl(QUrl(AUTHOR_URL));
            });
    }

    void openFile() {
        QString path = QFileDialog::getOpenFileName(this, "打开文件", QString(),
            "文本文件 (*.txt);;所有文件 (*.*)");
        if (path.isEmpty()) return;

        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "错误", "无法打开文件：" + f.errorString());
            return;
        }
        QByteArray data = f.readAll();
        f.close();

        QString content;
        QString usedEnc;
        if (data.startsWith("\xEF\xBB\xBF")) {
            content = QString::fromUtf8(data.mid(3));
            usedEnc = "utf-8-sig";
        }
        else {
            QStringList tries = { "utf-8", "gbk", "gb18030" };
            bool ok = false;
            for (auto& enc : tries) {
                QString s = decodeText(data, enc);
                if (!s.contains(QChar(0xFFFD))) {
                    content = s;
                    usedEnc = enc;
                    ok = true;
                    break;
                }
            }
            if (!ok) {
                content = QString::fromUtf8(data);
                usedEnc = "utf-8（可能包含乱码）";
            }
        }

        m_textEdit->setPlainText(content);
        m_tabs->setCurrentIndex(0);
        statusBar()->showMessage(QString("已打开（%1）: %2").arg(usedEnc, path), 5000);
    }

    void saveTextFile() {
        QString title = "untitled.txt";
        auto* dlg = new SaveDialog(this, title, QDir::currentPath());
        if (dlg->exec() != QDialog::Accepted) { dlg->deleteLater(); return; }

        QString path = dlg->finalPath();
        QString encName = dlg->encoding();
        dlg->deleteLater();
        if (path.isEmpty()) return;

        QString err;
        QByteArray data = encodeText(m_textEdit->toPlainText(), encName, err);
        if (!err.isEmpty()) {
            QMessageBox::critical(this, "编码错误", err + "\n\n建议改用 utf-8 或 UTF-8 BOM。");
            return;
        }

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "错误", "无法写入：" + f.errorString());
            return;
        }
        f.write(data);
        f.close();
        statusBar()->showMessage(QString("已保存（%1）: %2").arg(encName, path), 5000);
    }

    void onToolChanged(int id) {
        m_canvas->setTool(static_cast<CanvasWidget::Tool>(id));
    }

    void onTextRequested(const QPoint& pos) {
        bool ok = false;
        QString s = QInputDialog::getText(this, "输入文字", "请输入要添加的文字：",
            QLineEdit::Normal, QString(), &ok);
        if (ok) m_canvas->addTextAt(pos, s);
    }

    void onClearCanvas() {
        if (QMessageBox::question(this, "确认", "确定要清空整张画布吗？") == QMessageBox::Yes)
            m_canvas->clearCanvas();
    }

    void chooseFgColor() {
        QColor c = QColorDialog::getColor(m_canvas->fgColor(), this, "选择画笔颜色");
        if (!c.isValid()) return;
        m_canvas->setFgColor(c);
        for (auto* btn : findChildren<QPushButton*>()) {
            if (btn->minimumWidth() == 36 && btn->minimumHeight() == 24 &&
                btn->styleSheet().contains("#000000")) {
                btn->setStyleSheet(QString("background:%1; border:1px solid #666;").arg(c.name()));
                break;
            }
        }
    }

    void chooseBgColor() {
        QColor c = QColorDialog::getColor(m_canvas->bgColor(), this, "选择填充颜色");
        if (!c.isValid()) return;
        m_canvas->setBgColor(c);
        for (auto* btn : findChildren<QPushButton*>()) {
            if (btn->minimumWidth() == 36 && btn->minimumHeight() == 24 &&
                btn->styleSheet().contains("#ffffff")) {
                btn->setStyleSheet(QString("background:%1; border:1px solid #666;").arg(c.name()));
                break;
            }
        }
    }

    void saveImage() {
        QString path = QFileDialog::getSaveFileName(this, "保存图片", QString(),
            "PNG 图片 (*.png);;JPEG 图片 (*.jpg *.jpeg);;所有文件 (*.*)");
        if (path.isEmpty()) return;
        if (QFileInfo(path).suffix().isEmpty()) path += ".png";

        QImage img = m_canvas->renderToImage();
        if (img.save(path))
            statusBar()->showMessage("图片已保存: " + path, 5000);
        else
            QMessageBox::critical(this, "失败", "保存图片失败");
    }

private:
    QTabWidget* m_tabs = nullptr;
    QTextEdit* m_textEdit = nullptr;
    CanvasWidget* m_canvas = nullptr;
};

// ============================================================
int main(int argc, char* argv[]) {
    // 1. 强制设置 Windows 任务栏独立图标标识
#ifdef Q_OS_WIN
    SetCurrentProcessExplicitAppUserModelID(L"mycompany.notepad.app.3.0");
#endif

    QApplication app(argc, argv);
    app.setApplicationName("NotePaint");

    // 2. 设置窗口图标（注意这里路径用相对路径，因为 CMake 已经帮你把图标拷贝到 exe 旁边了）
    app.setWindowIcon(QIcon("icon.ico"));

    MainWindow w;
    w.show();
    return app.exec();
}
