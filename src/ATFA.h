/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#ifndef ATFA_H
#define ATFA_H

#include <QtGui>
#include <QtWidgets>
#include <QMainWindow>

#include "Signal.h"
#include "Stream.h"
#include "widgets/LEDIndicatorWidget.h"
#include "dialogs/ChangeAlgorithmDialog.h"

/*

    TODO: refatorar o código, obedecendo:
    (1) adhere strictly à regra de que "cada classe representa uma e somente uma
        abstração"; em particular, as classes de implementação devem estar
        completamente separadas das classes de GUI. (e.g., nada de classe
        Stream tendo pointer pra LED)
    (2) Evita naked-new/delete (usa unique_ptr, shared_ptr, etc) -> dá inclusive
        pra usar os smart pointers pra QObjects, mesmo que as funções do Qt
        recebam em geral raw pointers pros QObjects, pq a gente pode
        simplesmente usar o unique_pointer::get(), que dá acesso ao raw pointer
        subjacente ao unique_ptr. (Obs: http://doc.qt.io/qt-5/objecttrees.html
        ou seja, na verdade não é pra usar smart pointer pra QObjects,
        pq eles são deletados automaticamente de acordo com a hierarquia de
        pais/filhos dos QObjects; basta apenas deletar o paizão)
    (3) As classes de implementação devem ter muito bem especificadas as suas
        invariantes, e documentar se essas invariantes são enforced pelo código
        ou pelo usuário da classe.
    (4) Pensar, para todas as classes, se elas devem ter ou não
        (e se não tiver, garantir que não tem usando =delete):
        default constructor, copy constructor, copy assignment,
        move constructor, move assignment, destructor;
        se quiser garantir método default, usa =default .
    (5) Garantir const-correctness de todos os métodos de todas as classes. Se
        um método parece que deveria ser const mas não pode ser por motivos
        de implementação então dos dois um: ou ele tem uma explicação muito boa
        pra constness dele ser anti-intuitiva, ou então a gente tem que mudar
        a implementação pra que seja const.
    (6) As classes de implementação devem estar todas cobertas por Unit Tests,
        e.g. para a Stream devemos ter pares in/out de WAVs gerados pelo MATLAB.
    (7) A Stream deve ser conectável tanto ao PortAudio quanto a um
        produtor/consumidor externo.
    (8) Usar C++14(17?), e verificar se compila no Clang, além do GCC.
    (9) Usar valarray pra tudo.
   (10) Usar enum classes ao invés de enums.
   (11) Especificar premissas/assumptions do código em termos de
        static_assert's etc. (estas especificações devem estar documentadas
        de acordo com o item (3) acima)
   (12) Usa {}-initializers em TUDO (cuidado com ctors que aceitam
        initializer_list)
   (13) Usa tipos com tamanho explícito em tudo (int_64_t, etc), especialmente
        nas interfaces de DSO
   (14) Usa somente QString, nada de std::string
   (15) Usa noexcept e virtual+override SEMPRE que for o caso (obs, dar uma
        olhada no Q_DECL_OVERRIDE---qual a diferença dele pro override?)
   (16) constexpr pow (CTUtils), no c++14, não precisa daquela recursão feiosa,
        pode implementar usando loop mesmo
   (17) Quanto às pequenas variáveis que são acessadas thread-unsafely na
        implementação atual: faz uma flag de compilação que indica se elas
        devem ser safe (std::atomic) ou se deve ser unsafe mesmo
   (18) Evita unsigned; lembra que, mesmo que a taxa fosse 100kHz,
        o tipo 'int_32_t' seria o suficiente pra endereçar approx 6 horas de
        amostras, ou seja, não tem necessidade de usar mais de 32 bits pra
        índice de amostra (que é o que o size_t faz)
   (19) Keep functions short
   (20) Don't Repeat Yourself
   (21) Usa o I18n do Qt: http://doc.qt.io/qt-5/i18n-source-translation.html
   (22) Doxygen! Ou então pesquisa qual outra ferramenta de documentação é
        melhor.
   (23) Não abusa do std::endl; '\n' é portável, e não força flush.
   (24) Usa namespaces anônimos (ao invés de `static') pra forçar linkage
        interno -> forçar internal linkage ajuda na modularização e na
        separation of concerns.
   (25) ctors devem ser 'explicit' a menos que haja uma boa razão para o
        contrário (principalmente se tiverem um argumento só)
   (26) métodos inline com corpo grande devem ser definidos fora da definição
        da classe, de acordo com TC++PL4Ed Sec. 16.2.8 (p. 461)

*/


class ATFA : public QMainWindow {

    Q_OBJECT

public:
    explicit ATFA(QWidget *parent = 0);

    Stream stream;
    PaStream *pastream;

    int get_delay();

    bool muted;

    int delay_min;
    constexpr static int delay_max = 500;

    void update_widgets();

private slots:
    // file
    void newscene();
    void open();
    void save();
    void save_as();
    void quit();

    // tools
    void change_syslatency();
    void benchmark_dso();

    // help
    void show_help();
    void about_atfa();
    void about_qt();

    // ui
    void flearn_on_toggled(bool t);
    void flearn_off_toggled(bool t);
    void flearn_vad_toggled(bool t);

    void zero_filter_clicked();

    void play_clicked();

    void delay_changed(int v);

    void vol_mute_toggled(bool t);

    void vol_changed(int v);

    void show_rir();
    void change_rir();

    void show_adapf();
    void change_adapf();


private:
    QAction *newscene_act;
    QAction *open_act;
    QAction *save_act;
    QAction *save_as_act;
    QAction *quit_act;
    QAction *syslatency_act;
    QAction *benchmark_act;
    QAction *show_help_act;
    QAction *about_atfa_act;
    QAction *about_qt_act;

    QMenu *file_menu;
    QMenu *tools_menu;
    QMenu *help_menu;

    QToolBar *toolbar;

    QWidget *main_widget;

    // left layout
    QGroupBox *flearn_group;
        QRadioButton *flearn_on_radio;
        QRadioButton *flearn_off_radio;
        QRadioButton *flearn_vad_radio;
    QPushButton *zero_button;
    QGroupBox *fout_group;
        QRadioButton *fout_on_radio;
        QRadioButton *fout_off_radio;
        QRadioButton *fout_vad_radio;
    // right layout
    QWidget *vad_indicator_widget;
        QLabel *vad_indicator_label;
        LEDIndicatorWidget *vad_indicator_led;
        QComboBox *vad_algorithm_combo;
    QPushButton *play_button;
    QWidget *delay_widget;
        QLabel *delay_label;
        QSlider *delay_slider;
        QSpinBox *delay_spin;
        QLabel *delay_units;
    QWidget *vol_widget;
        QLabel *vol_label;
        QPushButton *vol_mute_button;
        QSlider *vol_slider;
        QSpinBox *vol_spin;
    QWidget *rir_widget;
        QLabel *rir_label;
        QLabel *rir_type_label;
        QPushButton *rir_show_button;
        QPushButton *rir_change_button;
    QWidget *adapf_widget;
        QLabel *adapf_label;
        QLabel *adapf_file_label;
        QPushButton *adapf_show_button;
        QPushButton *adapf_change_button;

    QString scene_filename;

    void set_stream_rir(const Stream::container_t &h);
    void set_stream_rir(Signal h);
    void set_new_rir(Scene::RIR_source_t source, QString txt, QString filename);

    friend class ChangeAlgorithmDialog;

    void save_to_file(QString filename);

};

#endif // ATFA_H
