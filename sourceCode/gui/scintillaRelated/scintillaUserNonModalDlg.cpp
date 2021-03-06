
// Some parts of this code were inspired from Robert Umbehant's personal c compiler
// (http://www.codeproject.com/KB/cpp/Personal_C___Compiler.aspx)

#include "vrepMainHeader.h"
#include "scintillaUserNonModalDlg.h"
#include "v_rep_internal.h"
#include "luaScriptFunctions.h"
#include "vMessageBox.h"
#include <algorithm>
#include "app.h"
#include "v_repStringTable.h"
#include <SciLexer.h>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QShortcut>
#include "qdlgsimpleSearch.h"
#include "tinyxml2.h"
#include "tt.h"
#include "ttUtil.h"

Qt::WindowFlags CScintillaUserNonModalDlg::dialogStyle;

CScintillaUserNonModalDlg::CScintillaUserNonModalDlg(const std::string& xmlInfo,int scriptId,int sceneUniqueId,const char* callbackFunc,bool simScript,QWidget* pParent, Qt::WindowFlags f) : QDialog(pParent,dialogStyle)
{
    _scriptId=scriptId;
    _callbackFunc=callbackFunc;
    _simScript=simScript;
    _sceneUniqueId=sceneUniqueId;
    _open=true;
    _editable=true;
    _searchable=true;
    _isLua=false;
    _useVrepKeywords=false;
    _tabWidth=4;
    _size[0]=800;
    _size[1]=600;
    _position[0]=100;
    _position[1]=100;
    _title="Editor";

    _textColor=VRGB(50,50,50);
    _backgroundColor=VRGB(210,210,210);
    _selectionColor=VRGB(60,60,210);
    _keywords1Color=VRGB(152,0,0); //VRGB(100,100,210);
    _keywords2Color=VRGB(220,80,20); //VRGB(210,100,100);

    _commentColor=VRGB(0,140,0);
    _numberColor=VRGB(220,0,220);
    _stringColor=VRGB(255,255,0);
    _characterColor=VRGB(255,255,0);
    _operatorColor=VRGB(0,0,0);
    _preprocessorColor=VRGB(0,128,128);
    _identifierColor=VRGB(64,64,64);
    _wordColor=VRGB(0,0,255);
    _word4Color=VRGB(152,64,0);




    int tabWidth=4;
    std::string lexerStr("lua");


    tinyxml2::XMLDocument xmldoc;
    tinyxml2::XMLError error=xmldoc.Parse(xmlInfo.c_str(),xmlInfo.size());
    if(error==tinyxml2::XML_NO_ERROR)
    {
        tinyxml2::XMLElement* rootElement=xmldoc.FirstChildElement();
        const char* str=rootElement->Attribute("title");
        if (str!=NULL)
            setWindowTitle(str);
        str=rootElement->Attribute("textColor");
        _getColorFromString(str,_textColor);
        str=rootElement->Attribute("backgroundColor");
        _getColorFromString(str,_backgroundColor);
        str=rootElement->Attribute("selectionColor");
        _getColorFromString(str,_selectionColor);
        str=rootElement->Attribute("commentColor");
        _getColorFromString(str,_commentColor);
        str=rootElement->Attribute("numberColor");
        _getColorFromString(str,_numberColor);
        str=rootElement->Attribute("stringColor");
        _getColorFromString(str,_stringColor);
        str=rootElement->Attribute("characterColor");
        _getColorFromString(str,_characterColor);
        str=rootElement->Attribute("operatorColor");
        _getColorFromString(str,_operatorColor);
        str=rootElement->Attribute("preprocessorColor");
        _getColorFromString(str,_preprocessorColor);
        str=rootElement->Attribute("identifierColor");
        _getColorFromString(str,_identifierColor);
        str=rootElement->Attribute("wordColor");
        _getColorFromString(str,_wordColor);
        str=rootElement->Attribute("word4Color");
        _getColorFromString(str,_word4Color);

        str=rootElement->Attribute("tabWidth");
        if (str!=NULL)
            tt::stringToInt(str,tabWidth);
        str=rootElement->Attribute("size");
        if (str!=NULL)
        {
            std::string line(str);
            std::string w;
            if (tt::extractSpaceSeparatedWord(line,w))
            {
                tt::stringToInt(w.c_str(),_size[0]);
                if (tt::extractSpaceSeparatedWord(line,w))
                    tt::stringToInt(w.c_str(),_size[1]);
            }
        }
        str=rootElement->Attribute("position");
        if (str!=NULL)
        {
            std::string line(str);
            std::string w;
            if (tt::extractSpaceSeparatedWord(line,w))
            {
                tt::stringToInt(w.c_str(),_position[0]);
                if (tt::extractSpaceSeparatedWord(line,w))
                    tt::stringToInt(w.c_str(),_position[1]);
            }
        }
        rootElement->QueryBoolAttribute("searchable",&_searchable);
        rootElement->QueryBoolAttribute("editable",&_editable);
        rootElement->QueryBoolAttribute("isLua",&_isLua);
        rootElement->QueryBoolAttribute("useVrepKeywords",&_useVrepKeywords);
//      str=rootElement->Attribute("lexer");
//      if (str!=NULL)
//          lexerStr=str;

        tinyxml2::XMLElement* keyw1Element=rootElement->FirstChildElement("keywords1");
        if (keyw1Element!=NULL)
        {
            str=keyw1Element->Attribute("color");
            _getColorFromString(str,_keywords1Color);
            tinyxml2::XMLElement* w=keyw1Element->FirstChildElement("item");
            while (w!=NULL)
            {
                str=w->Attribute("word");
                if (str!=NULL)
                {
                    if (_allKeywords1.size()>0)
                        _allKeywords1+=" ";
                    _allKeywords1+=str;
                    SScintillaNMUserKeyword b;
                    b.keyword=str;
                    b.autocomplete=true;
                    w->QueryBoolAttribute("autocomplete",&b.autocomplete);
                    str=w->Attribute("calltip");
                    if (str!=NULL)
                        b.callTip=str;
                    _keywords1.push_back(b);
                }
                w=w->NextSiblingElement("item");
            }
        }
        if (_useVrepKeywords)
        {
            for (size_t i=0;simLuaCommands[i].name!="";i++)
            {
                if (_allKeywords1.size()>0)
                    _allKeywords1+=" ";
                _allKeywords1+=simLuaCommands[i].name;
                SScintillaNMUserKeyword b;
                b.keyword=simLuaCommands[i].name;
                b.autocomplete=simLuaCommands[i].autoComplete;
                b.callTip=simLuaCommands[i].callTip;
                _keywords1.push_back(b);
            }
            if (App::userSettings->getSupportOldApiNotation())
            {
                for (size_t i=0;simLuaCommandsOldApi[i].name!="";i++)
                {
                    if (_allKeywords1.size()>0)
                        _allKeywords1+=" ";
                    _allKeywords1+=simLuaCommandsOldApi[i].name;
                    SScintillaNMUserKeyword b;
                    b.keyword=simLuaCommandsOldApi[i].name;
                    b.autocomplete=simLuaCommandsOldApi[i].autoComplete;
                    b.callTip=simLuaCommandsOldApi[i].callTip;
                    _keywords1.push_back(b);
                }
            }
            for (size_t i=0;i<App::ct->luaCustomFuncAndVarContainer->allCustomFunctions.size();i++)
            {
                if (_allKeywords1.size()>0)
                    _allKeywords1+=" ";
                _allKeywords1+=App::ct->luaCustomFuncAndVarContainer->allCustomFunctions[i]->getFunctionName();
                SScintillaNMUserKeyword b;
                b.keyword=App::ct->luaCustomFuncAndVarContainer->allCustomFunctions[i]->getFunctionName();
                b.autocomplete=App::ct->luaCustomFuncAndVarContainer->allCustomFunctions[i]->getCallTips().size()>0;
                b.callTip=App::ct->luaCustomFuncAndVarContainer->allCustomFunctions[i]->getCallTips();
                _keywords1.push_back(b);
            }
        }

        tinyxml2::XMLElement* keyw2Element=rootElement->FirstChildElement("keywords2");
        if (keyw2Element!=NULL)
        {
            str=keyw2Element->Attribute("color");
            _getColorFromString(str,_keywords2Color);
            tinyxml2::XMLElement* w=keyw2Element->FirstChildElement("item");
            while (w!=NULL)
            {
                str=w->Attribute("word");
                if (str!=NULL)
                {
                    if (_allKeywords2.size()>0)
                        _allKeywords2+=" ";
                    _allKeywords2+=str;
                    SScintillaNMUserKeyword b;
                    b.keyword=str;
                    b.autocomplete=true;
                    w->QueryBoolAttribute("autocomplete",&b.autocomplete);
                    str=w->Attribute("calltip");
                    if (str!=NULL)
                        b.callTip=str;
                    _keywords2.push_back(b);

                }
                w=w->NextSiblingElement("item");
            }
        }
        if (_useVrepKeywords)
        {
            for (size_t i=0;simLuaVariables[i].name!="";i++)
            {
                if (_allKeywords2.size()>0)
                    _allKeywords2+=" ";
                _allKeywords2+=simLuaVariables[i].name;
                SScintillaNMUserKeyword b;
                b.keyword=simLuaVariables[i].name;
                b.autocomplete=simLuaVariables[i].autoComplete;
                b.callTip="";
                _keywords2.push_back(b);
            }
            if (App::userSettings->getSupportOldApiNotation())
            {
                for (size_t i=0;simLuaVariablesOldApi[i].name!="";i++)
                {
                    if (_allKeywords2.size()>0)
                        _allKeywords2+=" ";
                    _allKeywords2+=simLuaVariablesOldApi[i].name;
                    SScintillaNMUserKeyword b;
                    b.keyword=simLuaVariablesOldApi[i].name;
                    b.autocomplete=simLuaVariablesOldApi[i].autoComplete;
                    b.callTip="";
                    _keywords2.push_back(b);
                }
            }
            for (size_t i=0;i<App::ct->luaCustomFuncAndVarContainer->allCustomVariables.size();i++)
            {
                if (_allKeywords2.size()>0)
                    _allKeywords2+=" ";
                _allKeywords2+=App::ct->luaCustomFuncAndVarContainer->allCustomVariables[i]->getVariableName();
                SScintillaNMUserKeyword b;
                b.keyword=App::ct->luaCustomFuncAndVarContainer->allCustomVariables[i]->getVariableName();
                b.autocomplete=App::ct->luaCustomFuncAndVarContainer->allCustomVariables[i]->getHasAutoCompletion();
                b.callTip="";
                _keywords2.push_back(b);
            }
        }
    }

    setAttribute(Qt::WA_DeleteOnClose);
    _scintillaObject=new QsciScintilla;

    // Use following if using a QDialog!!
    QVBoxLayout *bl=new QVBoxLayout(this);
    bl->setContentsMargins(0,0,0,0);
    setLayout(bl);
    bl->addWidget(_scintillaObject);

// use following if using a QMainWindow!!!   setCentralWidget(_scintillaObject);

    QsciLexer* lexer=NULL;
    if (lexerStr.compare("lua")==0)
        lexer=new QsciLexerLua;
    /*
    if (lexerStr.compare("java")==0)
        lexer=new QsciLexerJava;
    if (lexerStr.compare("python")==0)
        lexer=new QsciLexerPython;
    if (lexerStr.compare("cpp")==0)
        lexer=new QsciLexerCPP;
    if (lexerStr.compare("html")==0)
        lexer=new QsciLexerHTML;
    if (lexerStr.compare("octave")==0)
        lexer=new QsciLexerOctave;
    */
    if (lexer!=NULL)
        _scintillaObject->setLexer(lexer);

    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETSTYLEBITS,(int)5);
    _scintillaObject->setTabWidth(tabWidth);
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETUSETABS,(int)0);

    if (_allKeywords1.size()>0)
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETKEYWORDS,(unsigned long)1,_allKeywords1.c_str());
    if (_allKeywords2.size()>0)
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETKEYWORDS,(unsigned long)2,_allKeywords2.c_str());

    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETMARGINWIDTHN,(unsigned long)0,(long)48); // Line numbers
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETMARGINWIDTHN,(unsigned long)1,(long)0); // Symbols
    connect(_scintillaObject,SIGNAL(SCN_CHARADDED(int)),this,SLOT(_charAdded(int)));
    connect(_scintillaObject,SIGNAL(SCN_UPDATEUI(int)),this,SLOT(_updateUi(int)));

    if (_searchable)
    {
        QShortcut* shortcut = new QShortcut(QKeySequence(tr("Ctrl+f", "Find")), this);
        connect(shortcut,SIGNAL(activated()), this, SLOT(_onFind()));
    }

    _setColorsAndMainStyles();
}

CScintillaUserNonModalDlg::~CScintillaUserNonModalDlg()
{
    // _scintillaObject is normally automatically destroyed!
    // delete _scintillaObject;
}

void CScintillaUserNonModalDlg::closeEvent(QCloseEvent *event)
{
    event->ignore();
    forceClose(NULL,NULL,NULL);
}

void CScintillaUserNonModalDlg::forceClose(std::string* txt,int pos[2],int size[2])
{
    int l=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETLENGTH);
    _textAtClosing.resize(l+1);
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETTEXT,(unsigned long)l+1,&_textAtClosing[0]);
    _textAtClosing.resize(l);
    QRect geom(geometry());
    _sizeAtClosing[0]=geom.width();
    _sizeAtClosing[1]=geom.height();
    _posAtClosing[0]=geom.x();
    _posAtClosing[1]=geom.y();
    _open=false;
    if (txt!=NULL)
    {
        txt[0]=_textAtClosing;
        _callbackFunc.clear();
    }
    if (pos!=NULL)
    {
        pos[0]=_posAtClosing[0];
        pos[1]=_posAtClosing[1];
    }
    if (size!=NULL)
    {
        size[0]=_sizeAtClosing[0];
        size[1]=_sizeAtClosing[1];
    }
}

void CScintillaUserNonModalDlg::setHandle(int h)
{
    _handle=h;
}

int CScintillaUserNonModalDlg::getHandle() const
{
    return(_handle);
}

int CScintillaUserNonModalDlg::getSceneUniqueId() const
{
    return(_sceneUniqueId);
}

int CScintillaUserNonModalDlg::getScriptId() const
{
    return(_scriptId);
}

std::string CScintillaUserNonModalDlg::getCallbackFunc() const
{
    return(_callbackFunc);
}

bool CScintillaUserNonModalDlg::getIsOpen() const
{
    return(_open);
}

bool CScintillaUserNonModalDlg::isAssociatedWithSimScript() const
{
    return(_simScript);
}

void CScintillaUserNonModalDlg::handleCallback()
{
    int stack=simCreateStack_internal();
    simPushStringOntoStack_internal(stack,_textAtClosing.c_str(),_textAtClosing.size());
    simPushInt32TableOntoStack_internal(stack,_posAtClosing,2);
    simPushInt32TableOntoStack_internal(stack,_sizeAtClosing,2);
    simCallScriptFunctionEx_internal(_scriptId,_callbackFunc.c_str(),stack);
    simReleaseStack_internal(stack);
    _callbackFunc.clear();
}

void CScintillaUserNonModalDlg::initialize(const char* text)
{
    move(_position[0],_position[1]);
    resize(_size[0],_size[1]);
    show();

    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETTEXT,(unsigned long)0,text);
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_EMPTYUNDOBUFFER); // Make sure the undo buffer is empty (otherwise the first undo will remove the whole script --> a bit confusing)
    if (!_editable)
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETREADONLY,(int)1);
}

void CScintillaUserNonModalDlg::showOrHideDlg(bool showState)
{
    if (_open)
    {
        if (showState)
            show();
        else
            hide();
    }
}




void CScintillaUserNonModalDlg::_setColorsAndMainStyles()
{ // backgroundStyle=0 Main script, 1=non-threaded, 2=threaded
    struct SScintillaColors
    {
        int iItem;
        unsigned int rgb;
    };
    const unsigned int black=VRGB(0,0,0);
    const unsigned int darkGrey=VRGB(70,70,70);
    const unsigned int white=VRGB(255,255,255);

    int fontSize=12;
    std::string theFont("Courier"); // since Scintilla 2.7.2 and Qt5.1.1, "Courier New" gives strange artifacts (with Mac and Linux)!
#ifdef MAC_VREP
    fontSize=16; // bigger fonts here
#endif
    if (App::userSettings->scriptEditorFontSize!=-1)
        fontSize=App::userSettings->scriptEditorFontSize;
    if (App::userSettings->scriptEditorFont.length()!=0)
        theFont=App::userSettings->scriptEditorFont;
#ifndef MAC_VREP
    if (App::sc>1)
        fontSize*=2;
#endif

    _setAStyle(QsciScintillaBase::STYLE_DEFAULT,_textColor,_backgroundColor,fontSize,theFont.c_str()); // set global default style

    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETCARETFORE,(unsigned long)black);
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_STYLECLEARALL); // set all styles
    _setAStyle(QsciScintillaBase::STYLE_LINENUMBER,white,darkGrey);
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETSELBACK,(unsigned long)1,(long)_selectionColor); // selection color

    _setAStyle(SCE_LUA_WORD2,_keywords1Color,_backgroundColor);
    _setAStyle(SCE_LUA_WORD3,_keywords2Color,_backgroundColor);

    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_INDICSETSTYLE,(unsigned long)20,(long)QsciScintillaBase::INDIC_STRAIGHTBOX);
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_INDICSETALPHA,(unsigned long)20,(long)160);
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_INDICSETFORE,(unsigned long)20,(long)_selectionColor);


    if (_isLua)
    {
        static SScintillaColors syntaxColors[]=
        {
            {SCE_LUA_COMMENT,_commentColor},
            {SCE_LUA_COMMENTLINE,_commentColor},
            {SCE_LUA_COMMENTDOC,_commentColor},
            {SCE_LUA_NUMBER,_numberColor},
            {SCE_LUA_STRING,_stringColor},
            {SCE_LUA_LITERALSTRING,_stringColor},
            {SCE_LUA_CHARACTER,_characterColor},
            {SCE_LUA_OPERATOR,_operatorColor},
            {SCE_LUA_PREPROCESSOR,_preprocessorColor},
            {SCE_LUA_WORD,_wordColor},
            // {SCE_LUA_WORD2,colWord2},
            // {SCE_LUA_WORD3,colWord3},
            {SCE_LUA_WORD4,_word4Color},
            {SCE_LUA_IDENTIFIER,_identifierColor},
            {-1,0}
        };

        // Set syntax colors
        for (int i=0;syntaxColors[i].iItem!=-1;i++)
            _setAStyle(syntaxColors[i].iItem,syntaxColors[i].rgb,_backgroundColor);
    }
}

void CScintillaUserNonModalDlg::_setAStyle(int style,unsigned int fore,unsigned int back,int size,const char *face)
{
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_STYLESETFORE,(unsigned long)style,(long)fore);
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_STYLESETBACK,(unsigned long)style,(long)back);
    if (size>=1)
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_STYLESETSIZE,(unsigned long)style,(long)size);
    if (face)
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_STYLESETFONT,(unsigned long)style,face);
}

void CScintillaUserNonModalDlg::_prepAutoCompletionList(const std::string& txt)
{
    _autoCompletionList.clear();
    std::vector<std::string> t;
    std::map<std::string,bool> map;

    std::string ttxt(txt);
    bool hasDot=(ttxt.find('.')!=std::string::npos);

    for (size_t i=0;i<_keywords1.size();i++)
    {
        if ((_keywords1[i].autocomplete)&&(_keywords1[i].keyword.size()>=txt.size())&&(_keywords1[i].keyword.compare(0,txt.size(),txt)==0))
        {
            std::string n(_keywords1[i].keyword);
            if (!hasDot)
            {
                size_t dp=n.find('.');
                if (dp!=std::string::npos)
                    n.erase(n.begin()+dp,n.end()); // we only push the text up to the dot
            }
            std::map<std::string,bool>::iterator it=map.find(n);
            if (it==map.end())
            {
                map[n]=true;
                t.push_back(n);
            }
        }
    }

    for (size_t i=0;i<_keywords2.size();i++)
    {
        if ((_keywords2[i].autocomplete)&&(_keywords2[i].keyword.size()>=txt.size())&&(_keywords2[i].keyword.compare(0,txt.size(),txt)==0))
        {
            std::string n(_keywords2[i].keyword);
            if (!hasDot)
            {
                size_t dp=n.find('.');
                if (dp!=std::string::npos)
                    n.erase(n.begin()+dp,n.end()); // we only push the text up to the dot
            }
            std::map<std::string,bool>::iterator it=map.find(n);
            if (it==map.end())
            {
                map[n]=true;
                t.push_back(n);
            }
        }
    }

    std::sort(t.begin(),t.end());

    for (size_t i=0;i<t.size();i++)
    {
        _autoCompletionList+=t[i];
        if (i!=t.size()-1)
            _autoCompletionList+=' ';
    }
}

std::string CScintillaUserNonModalDlg::_getCallTip(const char* txt) const
{ 
    size_t l=strlen(txt);

    for (size_t i=0;i<_keywords1.size();i++)
    {
        if ( (_keywords1[i].keyword.size()==l)&&(_keywords1[i].keyword.compare(txt)==0) )
            return(_keywords1[i].callTip);
    }

    for (size_t i=0;i<_keywords2.size();i++)
    {
        if ( (_keywords2[i].keyword.size()==l)&&(_keywords2[i].keyword.compare(txt)==0) )
            return(_keywords2[i].callTip);
    }

    return("");
}

void CScintillaUserNonModalDlg::_updateUi(int updated)
{
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETINDICATORCURRENT,(int)20);

    int totTextLength=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETLENGTH);
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_INDICATORCLEARRANGE,(unsigned long)0,(long)totTextLength);

    int txtL=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETSELTEXT,(unsigned long)0,(long)0)-1;
    if (txtL>=1)
    {
        int selStart=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETSELECTIONSTART);

        char* txt=new char[txtL+1];
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETSELTEXT,(unsigned long)0,txt);

        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETSEARCHFLAGS,(int)QsciScintillaBase::SCFIND_MATCHCASE|QsciScintillaBase::SCFIND_WHOLEWORD);
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETTARGETSTART,(int)0);
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETTARGETEND,(int)totTextLength);

        int p=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_SEARCHINTARGET,(unsigned long)txtL,txt);
        while (p!=-1)
        {
            if (p!=selStart)
                _scintillaObject->SendScintilla(QsciScintillaBase::SCI_INDICATORFILLRANGE,(unsigned long)p,(long)strlen(txt));
            _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETTARGETSTART,(int)p+1);
            _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETTARGETEND,(int)totTextLength);
            p=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_SEARCHINTARGET,(unsigned long)txtL,txt);
        }
        delete[] txt;
    }
}

void CScintillaUserNonModalDlg::_onFind()
{
    int txtL=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETSELTEXT,(unsigned long)0,(long)0)-1;
    if (txtL>=1)
    {
        char* txt=new char[txtL+1];
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETSELTEXT,(unsigned long)0,txt);
        _findText(txt,CQDlgSimpleSearch::matchCase);
        delete[] txt;
    }
    else
    {
        CQDlgSimpleSearch simpleSearch(this);
        simpleSearch.init();
        simpleSearch.makeDialogModal();
        if (CQDlgSimpleSearch::textToSearch.length()!=0)
            _findText(CQDlgSimpleSearch::textToSearch.c_str(),CQDlgSimpleSearch::matchCase);
    }
}

void CScintillaUserNonModalDlg::_getColorFromString(const char* txt,unsigned int& col) const
{
    if (txt!=NULL)
    {
        std::string str(txt);
        int r,g,b;
        std::string line(txt);
        std::string w;
        if (tt::extractSpaceSeparatedWord(line,w))
        {
            tt::stringToInt(w.c_str(),r);
            if (tt::extractSpaceSeparatedWord(line,w))
            {
                tt::stringToInt(w.c_str(),g);
                if (tt::extractSpaceSeparatedWord(line,w))
                {
                    tt::stringToInt(w.c_str(),b);
                    col=VRGB(r,g,b);
                }
            }
        }
    }
}

void CScintillaUserNonModalDlg::_findText(const char* txt,bool caseSensitive)
{
    int totTextLength=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETLENGTH);
    int caseInfo=0;
    if (caseSensitive)
        caseInfo=QsciScintillaBase::SCFIND_MATCHCASE;
    int txtL=(int)strlen(txt);
    int pos=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETSELECTIONSTART);

    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETSEARCHFLAGS,(int)caseInfo);
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETTARGETSTART,(int)pos+1);
    _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETTARGETEND,(int)totTextLength);

    int p=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_SEARCHINTARGET,(unsigned long)txtL,txt);
    if (p==-1)
    {
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETSEARCHFLAGS,(int)caseInfo);
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETTARGETSTART,(int)0);
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETTARGETEND,(int)totTextLength);

        p=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_SEARCHINTARGET,(unsigned long)txtL,txt);
    }
    if (p!=-1)
        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_SETSEL,(unsigned long)p,(long)p+txtL);
}

void CScintillaUserNonModalDlg::_charAdded(int charAdded)
{
    if (_scintillaObject->SendScintilla(QsciScintillaBase::SCI_AUTOCACTIVE)!=0)
    { // Autocomplete is active
        if (charAdded=='(')
            _scintillaObject->SendScintilla(QsciScintillaBase::SCI_AUTOCCANCEL);
    }
    if (_scintillaObject->SendScintilla(QsciScintillaBase::SCI_AUTOCACTIVE)==0)
    { // Autocomplete is not active
        if (_scintillaObject->SendScintilla(QsciScintillaBase::SCI_CALLTIPACTIVE)!=0)
        { // CallTip is active
        }
        else
        { // Calltip is not active
            if ( (charAdded=='(')||(charAdded==',') )
            { // Do we need to activate a calltip?

                char linebuf[1000];
                int current=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETCURLINE,(unsigned long)999,linebuf);
                int pos=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETCURRENTPOS);
                linebuf[current]='\0';
                std::string line(linebuf);
                // 1. Find '('. Not perfect, will also detect e.g. "(" or similar
                int cnt=0;
                int pahr=-1;
                for (pahr=current-1;pahr>0;pahr--)
                {
                    if (line[pahr]==')')
                        cnt--;
                    if (line[pahr]=='(')
                    {
                        cnt++;
                        if (cnt==1)
                            break;
                    }
                }
                if ( (cnt==1)&&(pahr>0) )
                { // 2. Get word
                    int spaceCnt=0;
                    int startword=pahr-1;
                    int endword=startword;
                    while ((startword>=0)&&(isalpha(line[startword])||isdigit(line[startword])||(line[startword]=='_')||(line[startword]=='.')||((line[startword]==' ')&&(spaceCnt>=0)) ))
                    {
                        if (line[startword]==' ')
                        {
                            if ( (spaceCnt==0)&&(endword!=startword) )
                                break;
                            spaceCnt++;
                            endword--;
                        }
                        else
                        {
                            if (spaceCnt>0)
                                spaceCnt=-spaceCnt;
                        }
                        startword--;
                    }
                    std::string s;
                    if (startword!=endword)
                    {
                        s.assign(line.begin()+startword+1,line.begin()+endword+1);
                        s=_getCallTip(s.c_str());
                    }
                    if (s!="")
                    {

                        int fontSize=12-4;
                        std::string theFont("Courier"); // since Scintilla 2.7.2 and Qt5.1.1, "Courier New" gives strange artifacts (with Mac and Linux)!
#ifdef MAC_VREP
                        fontSize=16-4; // bigger fonts here
#endif
                        if (App::userSettings->scriptEditorFontSize!=-1)
                            fontSize=App::userSettings->scriptEditorFontSize-4;
                        if (App::userSettings->scriptEditorFont.length()!=0)
                            theFont=App::userSettings->scriptEditorFont;
#ifndef MAC_VREP
//  if (App::sc>1)
//      fontSize*=2;
#endif
                        _setAStyle(QsciScintillaBase::STYLE_CALLTIP,VRGB(0,0,0),VRGB(255,255,255),fontSize,theFont.c_str());
                        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_CALLTIPUSESTYLE,(int)0);

                        int cursorPosInPixelsFromLeftWindowBorder=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_POINTXFROMPOSITION,(int)0,(unsigned long)pos);
                        int callTipWidthInPixels=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_TEXTWIDTH,QsciScintillaBase::STYLE_CALLTIP,s.c_str());
                        int charWidthInPixels=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_TEXTWIDTH,QsciScintillaBase::STYLE_DEFAULT,"0");
                        int callTipWidthInChars=callTipWidthInPixels/charWidthInPixels;
                        int cursorPosInCharsFromLeftWindowBorder=-5+cursorPosInPixelsFromLeftWindowBorder/charWidthInPixels; // 5 is the width in chars of the left border (line counting)
                        int cursorPosInCharsFromLeftBorder=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETCOLUMN,(int)pos);
                        unsigned off=-SIM_MIN(cursorPosInCharsFromLeftWindowBorder,cursorPosInCharsFromLeftBorder);
                        if (callTipWidthInChars<SIM_MIN(cursorPosInCharsFromLeftWindowBorder,cursorPosInCharsFromLeftBorder))
                            off=-callTipWidthInChars;

                        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_CALLTIPSHOW,(unsigned long)pos+off,s.c_str());
                    }
                }
            }
            else
            { // Do we need to activate autocomplete?
                int p=-1+_scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETCURRENTPOS);
                if (p>=2)
                {
                    char linebuf[1000];
                    int current=_scintillaObject->SendScintilla(QsciScintillaBase::SCI_GETCURLINE,(unsigned long)999,linebuf);
                    linebuf[current]='\0';
                    std::string line(linebuf);
                    int ind=(int)line.size()-1;
                    int cnt=0;
                    std::string theWord;
                    while ((ind>=0)&&(isalpha(line[ind])||isdigit(line[ind])||(line[ind]=='_')||(line[ind]=='.') ))
                    {
                        theWord=line[ind]+theWord;
                        ind--;
                        cnt++;
                    }
                    if (theWord.size()>=3)
                    {
                        _prepAutoCompletionList(theWord);
                        if (_autoCompletionList.size()!=0)
                        { // We need to activate autocomplete!
                            _scintillaObject->SendScintilla(QsciScintillaBase::SCI_AUTOCSETAUTOHIDE,(int)0);
                            _scintillaObject->SendScintilla(QsciScintillaBase::SCI_AUTOCSTOPS,(unsigned long)0," ()[]{}:;~`',=*-+/?!@#$%^&|\\<>\"");
//                        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_AUTOCSETMAXHEIGHT,(int)10); // it seems that SCI_AUTOCSETMAXHEIGHT and SCI_AUTOCSETMAXWIDTH are not implemented yet!
//                        _scintillaObject->SendScintilla(QsciScintillaBase::SCI_AUTOCSETMAXWIDTH,(int)0); // it seems that SCI_AUTOCSETMAXHEIGHT and SCI_AUTOCSETMAXWIDTH are not implemented yet!
                            _scintillaObject->SendScintilla(QsciScintillaBase::SCI_AUTOCSHOW,(unsigned long)cnt,&(_autoCompletionList[0]));
                        }
                    }
                }
            }
        }
    }
}
