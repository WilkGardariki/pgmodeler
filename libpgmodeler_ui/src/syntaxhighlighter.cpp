#include "syntaxhighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent, bool auto_redestaque) : QSyntaxHighlighter(parent)
{
 this->auto_rehighlight=auto_redestaque;
 configureAttributes();
}

SyntaxHighlighter::SyntaxHighlighter(QTextEdit *parent, bool auto_redestaque) : QSyntaxHighlighter(parent)
{
 this->auto_rehighlight=auto_redestaque;
 configureAttributes();
}

void SyntaxHighlighter::configureAttributes(void)
{
 conf_loaded=false;
 current_block=-1;
 curr_blk_info_count=0;

 if(auto_rehighlight)
 {
  connect(document(), SIGNAL(blockCountChanged(int)), this, SLOT(rehighlight(void)));
  connect(document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(validateTextModification(int,int,int)));
 }
}

void SyntaxHighlighter::validateTextModification(int, int rem, int ins)
{
 unsigned qtd;

 //Obtém a quantidade de informações de multilinha do bloco atual
 qtd=getMultiLineInfoCount(current_block);

 /* Caso a quantidade obtida seja diferente da quantidade capturada
    antes da entrada do texto pelo usuário, ou se não exista nenhuma
    informação de multilinha para o bloco atual porém foram inseridos
    ou removidos alguns caracteres, o documento passará por um redestaque
    completo pois os caracteres removidos ou inseridos podem interferir
    na forma de apresentação do destaque e por isso o documento como
    um todo precisa ser redestacado */
 if(qtd!=curr_blk_info_count ||
   (/*qtd==0 && qtd_info_bloco_atual==qtd &&*/ (ins > 0 || rem > 0)))
  rehighlight();
}

SyntaxHighlighter::MultiLineInfo *SyntaxHighlighter::getMultiLineInfo(int col_ini, int col_fim, int bloco)
{
 unsigned i, qtd;
 bool enc=false;
 MultiLineInfo *info=NULL;

 /* Varre o vetor de informações de multilinha a fim de verificar se
    os parâmetros passados estão dentro de um bloco (informação) multilinha */
 qtd=multi_line_infos.size();
 for(i=0; i < qtd; i++)
 {
  //Obtém uma informação
  info=multi_line_infos[i];

  /* Primeiramente é preciso verificar se o bloco passado está dentro dos limites
     estabelecidos pelos blocos inicial e final da informação de multilinha.
     É importante salientar que quando um bloco multilinha está em aberto, por exemplo,
     o usuário abriu um multilinha com '/ *' e não fechou com '* /' os atributos
     bloco_fim e col_fim da informação de multilinha possuirão o valor -1 até
     que o usuário feche o multilinha e o destacador identifique em que parte
     do texto se encontra esse fechamento */
  if(bloco >= info->start_block && (info->end_block < 0 || bloco <= info->end_block))
  {
   /* A seguir várias condições são testadas a fim de verificar se os parâmetros
      passados estão dentro de um bloco multilinha.*/

   /* Condição 1: O bloco passado é o mesmo da informação atual e a informação
                  atual trata de um multilinha porém aberto e fechado na mesma linha
                  de texto (info->bloco_ini==info->bloco_fim), será verificado
                  se o parâmetro col_ini e col_fim estão nos limites estabelecidos
                  pelas colunas inicial e final do informação */
   if(bloco==info->start_block && info->start_block==info->end_block)
    enc=(col_ini >= info->start_col && col_fim <= info->end_col);

   /* Condição 2: O bloco passado é o mesmo da informação atual e a informação
                  atual trata de um multilinha em aberto. Testa apenas se
                  a coluna inicial do parâmetro está após da coluna inicial da
                  informação. Isso indica que o texto digitado está após a
                  abertura do multilinha e consequentemente dentro do mesmo */
   else if(bloco == info->start_block)
    enc=(col_ini >= info->start_col);

   /* Condição 3: O bloco passado é o mesmo do bloco final da informação atual
                  e a informação atual trata de um multilinha fechado. Testa
                  apenas se a coluna final do parâmetro está antes da coluna
                  final da informação multilinha atual indicando que o texto
                  inserido está dentro do bloco multilinha */
   else if(info->end_block >=0 && bloco == info->end_block)
    enc=(col_fim <= info->end_col);

  /* Condição 4: A informação atual trata de um multilinha em aberto. Testa
                  apenas se o bloco passado está no mesmo bloco inicial da informação
                  ou após sem a necessidade de se testar as colunas e o bloco final.
                  Isso é feito pois se o texto é inserido no meio do multilinha
                  após o bloco de abertura, e como o bloco está em aberto, todo texto
                  inserido após o bloco de aberto será considerado um multibloco */
   else if(info->end_block < 0)
    enc=(bloco >= info->start_block);

  /* Condição 5: A informação atual trata de um multilinha fechado. Testa
                 apenas se o bloco passado está no meio do intervalo estabelecido
                 pelos blocos inicial e final da informação. Isso é feito
                 pois se o texto é inserido no meio do multilinha após o bloco de
                 abertura e antes do fechamento a linha do texto como um todo
                 é considerada um multibloco */
   else if(info->end_block >=0 && info->start_block!=info->end_block)
    enc=(bloco >= info->start_block && bloco <= info->end_block);
  }
 }

 if(enc)
  return(info);
 else
  return(NULL);
}

void SyntaxHighlighter::removeMultiLineInfo(int bloco)
{
 vector<MultiLineInfo *>::iterator itr, itr_end;

 itr=multi_line_infos.begin();
 itr_end=multi_line_infos.end();

 //Varre a lista de informações de multilinha
 while(itr!=itr_end)
 {
  //Caso a info atual tenha como bloco inicial o mesmo bloco passado
  if((*itr)->start_block==bloco)
  {
   //Remove a informação da memória
   delete(*itr);
   //Remove o elemento da lista e reinicia a varredura
   multi_line_infos.erase(itr);
   itr=multi_line_infos.begin();
   itr_end=multi_line_infos.end();
  }
  else
   itr++;
 }
}

unsigned SyntaxHighlighter::getMultiLineInfoCount(int bloco)
{
 vector<MultiLineInfo *>::iterator itr, itr_end;
 unsigned qtd=0;

 itr=multi_line_infos.begin();
 itr_end=multi_line_infos.end();

 /* Varre a lista de informação de multilinha e contabiliza
    aquelas as quais tem como bloco inicial o bloco passado */
 while(itr!=itr_end)
 {
  if((*itr)->start_block==bloco) qtd++;
  itr++;
 }

 return(qtd);
}

QString SyntaxHighlighter::identifyWordGroup(const QString &palavra, const QChar &chr_lookup, int idx, int &idx_comb, int &comp_combinacao)
{
 QRegExp expr;
 vector<QString>::iterator itr, itr_end;
 vector<QRegExp>::iterator itr_exp, itr_exp_end;
 vector<QRegExp> *vet_expr=NULL;
 QString grupo;
 bool combina=false, comb_parcial=false;
 MultiLineInfo *info=NULL;

 //Tenta obter uma informação de multilinha do bloco atual
 info=getMultiLineInfo(idx, idx, current_block);

 /* Caso o destacador estiver no meio do destaque de um bloco
    multi linha, é executado um procedimento diferente que
    é verificar se a palavra em questão não combina com uma
    das expressões finais do grupo indicando que o detaque
    do grupo deve ser interrompido após a palavra em questão */
 if(info)
 {
  grupo=info->group;

  /* Varre as expresões finais exclusivamente do grupo atual
     para verificar se a palavra em questão naõ é a de finalização
     do destaque do grupo */
  itr_exp=final_exprs[grupo].begin();
  itr_exp_end=final_exprs[grupo].end();
  comb_parcial=partial_comb[grupo];

  while(itr_exp!=itr_exp_end && !combina)
  {
   //Obtém a expressão do iterador atual
   expr=(*itr_exp);

    //Caso a expressão esteja configurara para combinação parcial
    if(comb_parcial)
    {
     //Obtém o índice inicial na palavra onde a expressão combina
     idx_comb=palavra.indexOf(expr);
     //Obtém o comprimento da combinação
     comp_combinacao=expr.matchedLength();
     /* Para se saber se a expressão combinou parcialmente com a palavra
        verifica se o indice da combinação é igual ou superior a zero */
     combina=(idx_comb >= 0);
    }
    else
    {
    /* Caso a expressão esteja com o tipo de padrão configurado como
       como FixedString indica que a mesmo não precisa ser tratada como
       uma expressão regular e sim como uma string comum, sendo assim
       a comparação feita é de string com string o que acaba se tornando
       mais rápido */
     if(expr.patternSyntax()==QRegExp::FixedString)
      combina=((expr.pattern().compare(palavra, expr.caseSensitivity())==0));
     else
      //Combina a expressão regular com a palavra
      combina=expr.exactMatch(palavra);

     if(combina)
     {
      idx_comb=0;
      comp_combinacao=palavra.length();
     }
    }

   if(combina && lookup_char.count(grupo) > 0 && chr_lookup!=lookup_char.at(grupo))
    combina=false;

   itr_exp++;
  }

  /* Caso a palavra combina com uma das expressões finais do grupo
     marca a informação multilinha obtida e configura a coluna final
     e bloco final da informção de bloco multilinha. */
  if(combina)
  {
   info->end_col=idx + idx_comb + comp_combinacao-1;
   info->end_block=current_block;
  }
  /* Caso o destacador permaneça num bloco de multilinha o índice
     de combinação e o comprimento da combinação serão, respectivamente,
     zero e o comprimento da palavra para forçar o destaque da palavra
     atual no grupo de multi linha atual indicando que o destacador ainda
     se encontra no bloco deste tipo */
  else
  {
   idx_comb=0;
   comp_combinacao=palavra.length();
  }

  return(grupo);
 }
 else
 {
  /* Obtém os iteradores do vetor de ordem de grupos
     para que as expressões dos mesmos sejam aplicadas
       palavra em questão com o intuito de verificar se
     a mesma faz parte do grupo */
  itr=groups_order.begin();
  itr_end=groups_order.end();

  while(itr!=itr_end && !combina)
  {
   //Obtém o grupo a partir do iterador
   grupo=(*itr);
   //Obtém o vetor de expressões iniciais do grupo
   vet_expr=&initial_exprs[grupo];
   itr++;

   //Varre a lista de expressões comparando com a palavra atual
   itr_exp=vet_expr->begin();
   itr_exp_end=vet_expr->end();
   comb_parcial=partial_comb[grupo];

   while(itr_exp!=itr_exp_end && !combina)
   {
    //Obtém a expressão a partir do iterador atual
    expr=(*itr_exp);

    //Caso a expressão esteja configurara para combinação parcial
    if(comb_parcial)
    {
     //Obtém o índice inicial na palavra onde a expressão combina
     idx_comb=palavra.indexOf(expr);
     //Obtém o comprimento da combinação
     comp_combinacao=expr.matchedLength();
     /* Para se saber se a expressão combinou parcialmente com a palavra
        verifica se o indice da combinação é igual ou superior a zero */
     combina=(idx_comb >= 0);
    }
    else
    {
    /* Caso a expressão esteja com o tipo de padrão configurado como
       como FixedString indica que a mesmo não precisa ser tratada como
       uma expressão regular e sim como uma string comum, sendo assim
       a comparação feita é de string com string o que acaba se tornando
       mais rápido */
     if(expr.patternSyntax()==QRegExp::FixedString)
      combina=((expr.pattern().compare(palavra, expr.caseSensitivity())==0));
     else
      //Combina a expressão regular com a palavra
      combina=expr.exactMatch(palavra);

     if(combina)
     {
      idx_comb=0;
      comp_combinacao=palavra.length();
     }
    }

   if(combina && lookup_char.count(grupo) > 0 && chr_lookup!=lookup_char.at(grupo))
    combina=false;

    itr_exp++;
   }

   /* Caso a palavra combine com uma das expressões do grupo
      verifica se o mesmo possui expressões finais o que indica
      que o grupo se trata de elementos de multi linha, ou seja,
      que o destaque do grupo se extende além da linha atual até
      um delimitador final do grupo ser encontrado. Desta
      forma aloca uma informção de multilinha com configurações iniciais */
   if(combina && final_exprs.count(grupo))
   {
    if(!info)
    {
     info=new MultiLineInfo;
     info->group=grupo;
     info->start_col=idx + idx_comb + comp_combinacao;
     info->start_block=current_block;
     multi_line_infos.push_back(info);
    }
   }
  }

  /* Caso a palavra não combine com nenhuma expressão de nenhum
     grupo força o método a retornar um nome de grupo vazio
     indicando que a palavra não combina com grupo algum */
  if(!combina) grupo="";

  return(grupo);
 }
}

void SyntaxHighlighter::rehighlight(void)
{
 MultiLineInfo *info=NULL;

 /* Remove todas as informações de multilinha
    pois durante o redestaque as mesmas são obtidas
    novamente */
 while(!multi_line_infos.empty())
 {
  info=multi_line_infos.back();
  multi_line_infos.pop_back();
  delete(info);
 }

 QSyntaxHighlighter::rehighlight();
}

void SyntaxHighlighter::highlightBlock(const QString &txt)
{
 /* Caso a linha esteja vazia ou consita apenas de uma
    linha em branco não executa o destacamento para não
    gerar processamento desnecessário. */
 current_block=currentBlock().blockNumber();

 if(!txt.isEmpty())
 {
  QString palavra, grupo, texto;
  unsigned i=0, tam, idx=0, i1;
  int idx_comb, comp_comb, tam_aux, col_ini;
  QChar chr_delim, chr_lookup;

  //Obtém o tamanho total do bloco de texto
  texto=txt + '\n';
  tam=texto.length();

  /* Remove as informações de multilinha do bloco atual
     para forçar a identifição de novas informações
     de multilinha no bloco atual */
  removeMultiLineInfo(current_block);

  do
  {
   /* Remove os caracteres que constam na lista de caracteres
      ignorandos enquanto estes aparecerem no texto */
   while(i < tam && ignored_chars.indexOf(texto[i])>=0) i++;

   //Caso o fim do texto não tenha sido alcançado
   if(i < tam)
   {
    /* Armazena a posição atual no texto pois é a partir dela que será
       feito o destaque da palavra extraída nas iterações abaixo */
    idx=i;

    //Caso o caractere atual seja um caractere separador de palavras
    if(word_separators.indexOf(texto[i])>=0)
    {
     /* Enquanto o caractere for um separado, o mesmo é concatenada junto
        com os demais separadores */
     while(i < tam && word_separators.indexOf(texto[i])>=0)
      palavra+=texto[i++];
    }
    //Caso o caractere atual seja um delimitador de palavras
    else if(word_delimiters.indexOf(texto[i])>=0)
    {
     //Armazena o caractere delimitador
     chr_delim=texto[i++];
     //Adiciona-o na palavra que está sendo extraída
     palavra+=chr_delim;

     /* Extrai todos os próximos caracteres concatenando-o  palavra,
        idenpendente da categoria destes, enquanto o caractere final
        delimitador de palavra não seja encontrado ou o fim do texto
        seja alcançado. */
     while(i < tam && chr_delim!=texto[i])
      palavra+=texto[i++];

     /* Caso o caractere delimitador final for encontrado concatena-o   palavra
        formando a palavra delimitada como um todo */
     if(i < tam && texto[i]==chr_delim)
     {
      palavra+=chr_delim;
      i++;
     }
    }
    /* Caso o caractere atual não se encaixe em nenhuma das categorias
       acima, será a executada a iteração padrão, que é extrair
       o caractere até alcançar um separadaor ou delimitador de palavra
       ou um caractere a ser ignorado */
    else
    {
     while(i < tam &&
           word_separators.indexOf(texto[i]) < 0 &&
           word_delimiters.indexOf(texto[i]) < 0 &&
           ignored_chars.indexOf(texto[i]) < 0)
     {
      palavra+=texto[i++];
     }
    }
   }

   /* Caso a palavra não esteja vazia, tenta localizar o grupo
      ao qual ela pertence */
   if(!palavra.isEmpty())
   {
    i1=i;
    while(i1 < tam && ignored_chars.indexOf(texto[i1])>=0) i1++;

    if(i1 < tam)
     chr_lookup=texto[i1];
    else
     chr_lookup='\0';

    //Obtém o grupo ao qual a palavra faz parte
    idx_comb=-1;
    comp_comb=0;
    grupo=identifyWordGroup(palavra,chr_lookup, idx, idx_comb, comp_comb);

    /* Caso o grupo foi identificado faz o destaque da palavra
       usando a posição inicial da palavra com o comprimento
       da mesma */
    if(!grupo.isEmpty())
    {
     col_ini=idx + idx_comb;
     setFormat(col_ini, comp_comb, formats[grupo]);
    }

    tam_aux=(idx_comb + comp_comb);
    if(idx_comb >=0 &&  tam_aux != palavra.length())
     i-=palavra.length() - tam_aux;

    //Limpa a palavra atual para obter uma nova
    palavra="";
   }
  }
  while(i < tam);

  /* Armazena a quantidade de informação de multilinhas no bloco atual,
     pois este atributo é usado para se saber se o documento passará
     por um redestaque ou não */
  curr_blk_info_count=getMultiLineInfoCount(current_block);
 }
}

bool SyntaxHighlighter::isConfigurationLoaded(void)
{
 return(conf_loaded);
}

void SyntaxHighlighter::clearConfiguration(void)
{
 initial_exprs.clear();
 final_exprs.clear();
 formats.clear();
 partial_comb.clear();
 groups_order.clear();
 word_sep_groups.clear();
 word_separators.clear();
 word_delimiters.clear();
 ignored_chars.clear();
 lookup_char.clear();

 configureAttributes();
}

void SyntaxHighlighter::loadConfiguration(const QString &nome_arq)
{
 if(nome_arq!="")
 {
  map<QString, QString> atributos;
  QString elem, tipo_exp, grupo;
  bool decl_grupos=false, sensivel_chr=false,
       negrito=false, italico=false,
       sublinhado=false, comb_parcial=false;
  QTextCharFormat formatacao;
  QRegExp exp_regular;
  QColor cor_fundo, cor_fonte;
  vector<QString>::iterator itr, itr_end;

  try
  {
   /* Caso o usuário tente carregar uma nova configuração na mesma instância,
      as configurações anteriores são descartadas */
   clearConfiguration();

   //Reinicia o parser XML para a leitura do arquivo
   XMLParser::restartParser();

   /* Montando o caminho padrão para localização do arquivo DTD que define a sintaxe
     do arquivo xml de destaque de código fonte. */
   XMLParser::setDTDFile(GlobalAttributes::CONFIGURATIONS_DIR +
                                GlobalAttributes::DIR_SEPARATOR +
                                GlobalAttributes::OBJECT_DTD_DIR +
                                GlobalAttributes::DIR_SEPARATOR +
                                GlobalAttributes::CODE_HIGHLIGHT_CONF +
                                GlobalAttributes::OBJECT_DTD_EXT,
                                GlobalAttributes::CODE_HIGHLIGHT_CONF);

   //Carrega o arquivo validando-o de acordo com a DTD informada
   XMLParser::loadXMLFile(nome_arq);

   //Acessa o elemento inicial do arquivo de destaque de código fonte
   if(XMLParser::accessElement(XMLParser::CHILD_ELEMENT))
   {
    do
    {
     if(XMLParser::getElementType()==XML_ELEMENT_NODE)
     {
      //Obtém o elemento atual
      elem=XMLParser::getElementName();

      //Obtém os separadores de palavras da linguagem
      if(elem==ParsersAttributes::WORD_SEPARATORS)
      {
       //Obtém os atributos do mesmo
       XMLParser::getElementAttributes(atributos);
       word_separators=atributos[ParsersAttributes::VALUE];
      }

      //Obtém os delimitadores de palavras da linguagem
      else if(elem==ParsersAttributes::WORD_DELIMITERS)
      {
       //Obtém os atributos do mesmo
       XMLParser::getElementAttributes(atributos);
       word_delimiters=atributos[ParsersAttributes::VALUE];
      }
      else if(elem==ParsersAttributes::IGNORED_CHARS)
      {
       //Obtém os atributos do mesmo
       XMLParser::getElementAttributes(atributos);
       ignored_chars=atributos[ParsersAttributes::VALUE];
      }

      /* Caso o elemento seja o que define a ordem de aplicação dos grupos
         de destaque (highlight-order). É neste bloco que são declarados
         os grupos usados para destacar o código-fonte. TODOS os grupos
         precisam ser declarados neste bloco antes de serem construídos
         caso contrário será disparado um erro. */
      else if(elem==ParsersAttributes::HIGHLIGHT_ORDER)
      {
       //Marca a flag indicando que os grupos estão sendo declarados
       decl_grupos=true;
       //Salva a posição atual do parser xml
       XMLParser::savePosition();
       /* Acesso o primeiro elemento filho da tag de ordem de destaque que
          no caso é uma tag de declaração de grupo <group> */
       XMLParser::accessElement(XMLParser::CHILD_ELEMENT);
       //Obtém o nome do elemento, no caso <group>
       elem=XMLParser::getElementName();
      }

      //Caso o elemento atual seja de construção de um grupo '<group>'
      if(elem==ParsersAttributes::GROUP)
      {
       //Obtém os atributos do mesmo
       XMLParser::getElementAttributes(atributos);
       //Armazena seu nome em uma variável auxiliar
       grupo=atributos[ParsersAttributes::NAME];

       /* Caso o parser estiver no bloco de declaração de grupos e não no bloco
          de construção dos mesmos, algumas validações serão executadas. */
       if(decl_grupos)
       {
        /* 1ª Validação: Verifica se o grupo já não foi declarando anteriormente,
                         para isso o mesmo é buscado na lista que armazena a ordem
                         de aplicação dos grupos (ordem_grupos). Caso o mesmo seja
                         encontrado um erro é disparado. Um grupo é dito como localizado
                         na lista quando a chamada a função find() retorna o iterador
                         diferente do iterador final da lista 'ordem_grupos.end()' */
        if(find(groups_order.begin(), groups_order.end(), grupo)!=groups_order.end())
        {
         //Dispara o erro indicado que o grupo está sendo redeclarado
         throw Exception(Exception::getErrorMessage(ERR_REDECL_HL_GROUP).arg(grupo),
                       ERR_REDECL_HL_GROUP,__PRETTY_FUNCTION__,__FILE__,__LINE__);
        }
        /* 2ª Validação: Verifica se o grupo está sendo declarado e construído ao mesmo tempo no
                         bloco de declaração. Um grupo no bloco de declaração deve aparecer no
                         formato <group name='nome'/>, qualquer construção diferente da apresentada
                         seja com mais atributos ou elementos filhos é considerado que o grupo está
                         sendo construído em local inválido */
        else if(atributos.size() > 1 || XMLParser::hasElement(XMLParser::CHILD_ELEMENT))
        {
         throw Exception(Exception::getErrorMessage(ERR_DEF_INV_GROUP_DECL)
                       .arg(grupo).arg(ParsersAttributes::HIGHLIGHT_ORDER),
                       ERR_REDECL_HL_GROUP,__PRETTY_FUNCTION__,__FILE__,__LINE__);
        }

        /* Caso nenhum erro for disparado o grupo é adicionad  lista de
           ordem de aplicação dos grupos */
        groups_order.push_back(grupo);
       }
       /* Caso o parser estiver no bloco de construção de grupos e não no bloco
          de declaração dos mesmos, algumas validações iniciais serão executadas. */
       else
       {
        /* 1ª Validação: Verifica se o grupo está sendo construído pela segunda vez.
                         Para tal, verifica se o mapa de expressões do grupo foi criado
                         em alguma iteração anterior. Caso exista essa ocorrencia indica
                         que o grupo já foi construído anteriormente,
                         desta forma um erro será disparado ao usuário */
        if(initial_exprs.count(grupo)!=0)
        {
         //Dispara o erro ao usuário indicando construção duplicada
         throw Exception(Exception::getErrorMessage(ERR_DEF_DUPLIC_GROUP).arg(grupo),
                       ERR_DEF_DUPLIC_GROUP,__PRETTY_FUNCTION__,__FILE__,__LINE__);
        }
        /* 2ª Validação: Verifica se o grupo está sendo construído sem ter sido declarado.
                         Para tal, verifica se grupo que está sendo construído não existe
                         na lista de ordem de aplicação de grupos. Um grupo é dito como
                         não localizado na lista quando a chamada a função find() retorna
                         o iterador final da lista 'ordem_grupos.end() */
        else if(find(groups_order.begin(), groups_order.end(), grupo)==groups_order.end())
        {
         //Dispara o erro indicando que o grupo foi construído e não declarado
         throw Exception(Exception::getErrorMessage(ERR_DEF_NOT_DECL_GROUP)
                       .arg(grupo).arg(ParsersAttributes::HIGHLIGHT_ORDER),
                       ERR_DEF_NOT_DECL_GROUP,__PRETTY_FUNCTION__,__FILE__,__LINE__);
        }
        /* 3ª Validação: Verifica se o grupo possui elementos filhos. No bloco de construção
                         do grupo é necessário que ele possua pelo menos um filho '<element>'.
                         Caso ele não possua elementos deste tipo um erro é retornado ao usuário */
        else if(!XMLParser::hasElement(XMLParser::CHILD_ELEMENT))
        {
         throw Exception(Exception::getErrorMessage(ERR_DEF_EMPTY_GROUP).arg(grupo),
                       ERR_DEF_EMPTY_GROUP,__PRETTY_FUNCTION__,__FILE__,__LINE__);
        }

        //Obtém e armazena em variáveis os atributos do grupo que está sendo construído
        sensivel_chr=(atributos[ParsersAttributes::CASE_SENSITIVE]==ParsersAttributes::_TRUE_);
        italico=(atributos[ParsersAttributes::ITALIC]==ParsersAttributes::_TRUE_);
        negrito=(atributos[ParsersAttributes::BOLD]==ParsersAttributes::_TRUE_);
        sublinhado=(atributos[ParsersAttributes::UNDERLINE]==ParsersAttributes::_TRUE_);
        comb_parcial=(atributos[ParsersAttributes::PARTIAL_MATCH]==ParsersAttributes::_TRUE_);
        cor_fonte.setNamedColor(atributos[ParsersAttributes::FOREGROUND_COLOR]);
        cor_fundo.setNamedColor(atributos[ParsersAttributes::BACKGROUND_COLOR]);

        if(!atributos[ParsersAttributes::LOOKUP_CHAR].isEmpty())
         lookup_char[grupo]=atributos[ParsersAttributes::LOOKUP_CHAR][0];

        //Configura a formatação do grupo de acordo com os atributos obtidos
        formatacao.setFontItalic(italico);
        formatacao.setFontUnderline(sublinhado);

        if(negrito)
         formatacao.setFontWeight(QFont::Bold);
        else
         formatacao.setFontWeight(QFont::Normal);

        formatacao.setForeground(cor_fonte);
        formatacao.setBackground(cor_fundo);
        formats[grupo]=formatacao;

        //Salva a posição atual do parser e acesso os elementos filhos do grupo
        XMLParser::savePosition();
        XMLParser::accessElement(XMLParser::CHILD_ELEMENT);

        /* Configura a variável de expressão regular para ser sensível a
           caracteres (case sensitive) de acordo com o mesmo atributo
           obtido do xml */
        if(sensivel_chr)
         exp_regular.setCaseSensitivity(Qt::CaseSensitive);
        else
         exp_regular.setCaseSensitivity(Qt::CaseInsensitive);

        partial_comb[grupo]=comb_parcial;

        do
        {
         if(XMLParser::getElementType()==XML_ELEMENT_NODE)
         {
          //Obtém os atributos do elemento filho do grupo
          XMLParser::getElementAttributes(atributos);
          //Obtém o tipo do elemento
          tipo_exp=atributos[ParsersAttributes::TYPE];

          //Configura a expressão regular com o valor presente no atributo 'value' do elemento
          exp_regular.setPattern(atributos[ParsersAttributes::VALUE]);

          if(atributos[ParsersAttributes::REGULAR_EXP]==ParsersAttributes::_TRUE_)
           exp_regular.setPatternSyntax(QRegExp::RegExp);
          else
           exp_regular.setPatternSyntax(QRegExp::FixedString);

          /* A expressão regular configura será inserida na lista de expressões
             de acordo com o tipo do elemento */
          if(tipo_exp=="" ||
             tipo_exp==ParsersAttributes::EXP_SIMPLES ||
             tipo_exp==ParsersAttributes::INITIAL_EXP)
           initial_exprs[grupo].push_back(exp_regular);
          else
           final_exprs[grupo].push_back(exp_regular);
         }
        }
        while(XMLParser::accessElement(XMLParser::NEXT_ELEMENT));

        //Restaura a posição do parser para continuar a leitura dos próximos grupos
        XMLParser::restorePosition();
       }
      }
     }

     /* Após a inserção do grupo, verifica se existem outros grupos a serem
        declarados. Caso não existe, desmarca a flag de declaração de grupos
        e restaura a posição do parser para que o restante do arquivo possa
        ser lido */
     if(decl_grupos && !XMLParser::hasElement(XMLParser::NEXT_ELEMENT))
     {
      decl_grupos=false;
      XMLParser::restorePosition();
     }

    }
    while(XMLParser::accessElement(XMLParser::NEXT_ELEMENT));
   }

   /* Executa a validação final do carregamento do arquivo que consiste em
      verificar se algum grupo foi declarado porém não construído. Para
      isso, a lista de ordem de grupos é varrida e verifica-se se
      existem expressões para tal grupo. Caso não exista expressões
      para o grupo indica que o mesmo foi declarado e não foi construído */
   itr=groups_order.begin();
   itr_end=groups_order.end();

   while(itr!=itr_end)
   {
    grupo=(*itr);
    itr++;

    //Caso o número de expressões do grupo seja zero
    if(initial_exprs[grupo].size()==0)
    {
     //Dispara o erro indicando que o grupo foi declarado porém não construído
     throw Exception(Exception::getErrorMessage(ERR_GROUP_DECL_NOT_DEFINED).arg(grupo),
                   ERR_GROUP_DECL_NOT_DEFINED,__PRETTY_FUNCTION__,__FILE__,__LINE__);
    }
   }

   //Marca a flag indicando que a configuração foi carregada com sucesso
   conf_loaded=true;
  }
  catch(Exception &e)
  {
   //Captura e redireciona erros das demais bibliotecas
   throw Exception(e.getErrorMessage(),e.getErrorType(),__PRETTY_FUNCTION__,__FILE__,__LINE__, &e);
  }
 }
}
