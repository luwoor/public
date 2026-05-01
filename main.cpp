#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>

struct Chapter {
    QString title;
    QString content;
};

class NovelWriter : public QMainWindow {
    Q_OBJECT
public:
    NovelWriter(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("小说创作器");
        setMinimumSize(1200, 700);
        m_currentIndex = -1;
        m_isDark = false;
        buildUI();
        updateStyle();
        updateWordCount();
    }

private:
    QList<Chapter> m_chapters;
    int m_currentIndex;
    bool m_isDark;

    QLineEdit* w_name;
    QTextEdit* w_world;
    QTextEdit* w_char;
    QTextEdit* w_outline;
    QTextEdit* w_foreshadow;
    QTextEdit* w_inspire;

    QListWidget* chapterList;
    QLineEdit* editTitle;
    QTextEdit* editContent;
    QLabel* currentWord;
    QLabel* totalWord;

    void buildUI() {
        QWidget* central = new QWidget(this);
        setCentralWidget(central);
        QHBoxLayout* mainLay = new QHBoxLayout(central);
        mainLay->setContentsMargins(10,10,10,10);
        mainLay->setSpacing(8);

        // 左侧知识库
        QWidget* leftWid = new QWidget;
        QVBoxLayout* leftLay = new QVBoxLayout(leftWid);
        leftLay->setSpacing(6);

        auto addGroup = [&](const QString& title, QWidget*& w, bool isLine = false) {
            QGroupBox* gb = new QGroupBox(title);
            QVBoxLayout* gl = new QVBoxLayout(gb);
            if(isLine) {
                w = new QLineEdit;
                gl->addWidget(w);
            } else {
                w = new QTextEdit;
                gl->addWidget(w);
            }
            leftLay->addWidget(gb);
        };

        addGroup("📖 小说名",     *(QWidget**)&w_name,   true);
        addGroup("🌍 世界观",     *(QWidget**)&w_world);
        addGroup("👤 人物",       *(QWidget**)&w_char);
        addGroup("📝 大纲",       *(QWidget**)&w_outline);
        addGroup("🔍 伏笔",       *(QWidget**)&w_foreshadow);
        addGroup("💡 灵感",       *(QWidget**)&w_inspire);

        // 中间章节
        QWidget* midWid = new QWidget;
        QVBoxLayout* midLay = new QVBoxLayout(midWid);
        midLay->addWidget(new QLabel("📖 章节列表"));
        chapterList = new QListWidget;
        midLay->addWidget(chapterList);

        // 右侧编辑
        QWidget* rightWid = new QWidget;
        QVBoxLayout* rightLay = new QVBoxLayout(rightWid);
        rightLay->setSpacing(8);

        QHBoxLayout* btnLay = new QHBoxLayout;
        QPushButton* btnSaveChap = new QPushButton("💾 保存章节");
        QPushButton* btnSavePro  = new QPushButton("📦 保存项目");
        QPushButton* btnExport   = new QPushButton("📄 导出TXT");
        QPushButton* btnReset    = new QPushButton("🔄 一键初始化");
        QPushButton* btnTheme    = new QPushButton("🌓 明暗切换");
        btnLay->addWidget(btnSaveChap);
        btnLay->addWidget(btnSavePro);
        btnLay->addWidget(btnExport);
        btnLay->addWidget(btnReset);
        btnLay->addWidget(btnTheme);

        editTitle = new QLineEdit;
        editTitle->setPlaceholderText("输入章节标题...");
        editContent = new QTextEdit;
        editContent->setPlaceholderText("在这里写章节正文……");

        QHBoxLayout* wordLay = new QHBoxLayout;
        currentWord = new QLabel("当前章节：0 字");
        totalWord   = new QLabel("全书总字数：0 字");
        wordLay->addWidget(currentWord);
        wordLay->addWidget(totalWord);

        rightLay->addLayout(btnLay);
        rightLay->addWidget(editTitle);
        rightLay->addWidget(editContent);
        rightLay->addLayout(wordLay);

        mainLay->addWidget(leftWid,   4);
        mainLay->addWidget(midWid,    2);
        mainLay->addWidget(rightWid,   6);

        connect(btnSaveChap, &QPushButton::clicked, this, &NovelWriter::saveChapter);
        connect(btnSavePro,  &QPushButton::clicked, this, &NovelWriter::saveProject);
        connect(btnExport,  &QPushButton::clicked, this, &NovelWriter::exportTxt);
        connect(btnReset,   &QPushButton::clicked, this, &NovelWriter::resetAll);
        connect(btnTheme,   &QPushButton::clicked, this, &NovelWriter::toggleTheme);
        connect(chapterList, &QListWidget::itemClicked, this, &NovelWriter::loadChapter);
        connect(editContent, &QTextEdit::textChanged, this, &NovelWriter::updateWordCount);
    }

    void updateStyle() {
        if(!m_isDark) {
            setStyleSheet(R"(
                QMainWindow{background:#f5f7fa;}
                QGroupBox{border:1px solid #d0d7e3;border-radius:8px;margin-top:6px;}
                QLineEdit,QTextEdit,QListWidget{background:#fff;color:#2c3e50;border:1px solid #d0d7e3;border-radius:6px;padding:6px;}
                QPushButton{background:#2c5282;color:#fff;padding:8px;border-radius:6px;}
            )");
        } else {
            setStyleSheet(R"(
                QMainWindow{background:#1a1d24;}
                QGroupBox{border:1px solid #444;border-radius:8px;margin-top:6px;}
                QLineEdit,QTextEdit,QListWidget{background:#272b33;color:#e8e8e8;border:1px solid #444;border-radius:6px;padding:6px;}
                QPushButton{background:#4a7fc2;color:#fff;padding:8px;border-radius:6px;}
            )");
        }
    }

    void updateWordCount() {
        QString currText = editContent->toPlainText();
        int curr = currText.remove(" ").remove("\n").length();
        currentWord->setText(QString("当前章节：%1 字").arg(curr));

        int total = 0;
        for(auto& ch : m_chapters) {
            QString chapText = ch.content;
            total += chapText.remove(" ").remove("\n").length();
        }
        totalWord->setText(QString("全书总字数：%1 字").arg(total));
    }

    void saveChapter() {
        QString t = editTitle->text().trimmed();
        QString c = editContent->toPlainText();
        if(t.isEmpty()){
            QMessageBox::warning(this, "提示", "请输入章节标题！");
            return;
        }
        if(m_currentIndex < 0) {
            m_chapters.append({t,c});
        } else {
            m_chapters[m_currentIndex] = {t,c};
        }
        refreshChapterList();
        editTitle->clear();
        editContent->clear();
        m_currentIndex = -1;
        QMessageBox::information(this, "成功", "✅ 章节保存成功！");
    }

    void refreshChapterList() {
        chapterList->clear();
        for(int i=0; i<m_chapters.size(); i++) {
            chapterList->addItem(QString("第%1章 %2").arg(i+1).arg(m_chapters[i].title));
        }
        updateWordCount();
    }

    void loadChapter(QListWidgetItem* item) {
        m_currentIndex = chapterList->row(item);
        Chapter& ch = m_chapters[m_currentIndex];
        editTitle->setText(ch.title);
        editContent->setPlainText(ch.content);
    }

    void exportTxt() {
        QString name = w_name->text().trimmed();
        if(name.isEmpty()) name = "未命名小说";
        QString txt = QString("《%1》\r\n\r\n").arg(name);
        for(int i=0; i<m_chapters.size(); i++) {
            auto& ch = m_chapters[i];
            txt += QString("第%1章 %2\r\n\r\n").arg(i+1).arg(ch.title);
            txt += ch.content + "\r\n\r\n\r\n";
        }
        QString path = QFileDialog::getSaveFileName(this, "导出TXT", name + ".txt", "*.txt");
        if(path.isEmpty()) return;
        QFile f(path);
        if(f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out << txt;
            f.close();
            QMessageBox::information(this, "成功", "✅ 导出TXT成功！");
        }
    }

    void saveProject() {
        QJsonObject obj;
        obj["novelName"]  = w_name->text();
        obj["worldview"]  = w_world->toPlainText();
        obj["characters"] = w_char->toPlainText();
        obj["outline"]    = w_outline->toPlainText();
        obj["foreshadow"] = w_foreshadow->toPlainText();
        obj["inspire"]    = w_inspire->toPlainText();

        QJsonArray arr;
        for(auto& ch : m_chapters) {
            QJsonObject co;
            co["title"]   = ch.title;
            co["content"] = ch.content;
            arr.append(co);
        }
        obj["chapters"] = arr;

        QJsonDocument doc(obj);
        QString path = QFileDialog::getSaveFileName(this, "保存项目", "小说项目.json", "*.json");
        if(path.isEmpty()) return;
        QFile f(path);
        if(f.open(QIODevice::WriteOnly)) {
            f.write(doc.toJson());
            f.close();
            QMessageBox::information(this, "成功", "✅ 项目保存成功！");
        }
    }

    void resetAll() {
        if(QMessageBox::question(this, "确认", "⚠️ 确定要清空所有内容吗？") != QMessageBox::Yes)
            return;
        w_name->clear();
        w_world->clear();
        w_char->clear();
        w_outline->clear();
        w_foreshadow->clear();
        w_inspire->clear();
        m_chapters.clear();
        refreshChapterList();
        editTitle->clear();
        editContent->clear();
    }

    void toggleTheme() {
        m_isDark = !m_isDark;
        updateStyle();
    }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    NovelWriter w;
    w.show();
    return a.exec();
}

#include "main.moc"