# Chinese Postman Problem Solver

## 問題設定について

[中国人郵便配達問題 - Wikipedia](https://ja.wikipedia.org/wiki/%E4%B8%AD%E5%9B%BD%E4%BA%BA%E9%83%B5%E4%BE%BF%E9%85%8D%E9%81%94%E5%95%8F%E9%A1%8C)

## 動かし方

### 0. 必要なもの

動かすまでの準備が多いです。また導入の敷居が高いものも多く使っています。改善したいところです。

-   Ruby [http://www.ruby-lang.org/](http://www.ruby-lang.org/)  
    ファイル操作の簡易化のために利用しています。  
    Windows環境の場合、[RubyInstaller](http://rubyinstaller.org/)およびそのアドオンの「DevKit」を用いると、次に述べるMinGW環境がインストールできます。
-   （Windowsの場合）MinGW [http://www.mingw.org/](http://www.mingw.org/)  
    Unix系OSを想定したコードになっています。Windows環境においてはMinGWの利用を前提としています。
-   Boost C++ Libraries [http://www.boost.org/](http://www.boost.org/)  
    コンパイル済みのバージョンでなくても大丈夫です。Windowsの場合、ダウンロードして適当なフォルダに展開するだけでもよいです。
-   GLPK [https://www.gnu.org/software/glpk/](https://www.gnu.org/software/glpk/)  
    Windows環境の場合、上記MinGWを用いて、GLPKをソースコードからコンパイルしてください（注1）。"GLPK for Windows"での動作は未検証です。

（注1）これをどうすればよいか見当の付かない方には、環境構築全般も大変かと思います。参考までに、参考になるページをご紹介します。  
[「./configure」、「make」、「make install」の意味 - lovebug.jp](http://www.lovebug.jp/index.php?%E3%80%8C.%2Fconfigure%E3%80%8D%E3%80%81%E3%80%8Cmake%E3%80%8D%E3%80%81%E3%80%8Cmake%20install%E3%80%8D%E3%81%AE%E6%84%8F%E5%91%B3)

### 1. まずはビルドする

まず、Makefile内に記載されている`BOOST`と`GLPKDEVEL`のパスを書き換えます。`BOOST`はboostをインストールした先のディレクトリ、`GLPKDEVEL`はGLPKをインストールした先の**一つ上**のディレクトリ（例えば、libglpk.aが/usr/local/libに入っているならば`GLPKDEVEL=/usr/local`）を指定します。

あとは

    make

コマンドでビルドが始まります。`DivideByBridge.exe`と`SolveChinesePostman.exe`が問題なく生成されれば成功です。

### 2. 普通に解く

    ./SolveChinesePostman.exe jrhokkaido.edges

のように実行します（Windows環境の場合、最初の`./`は不要）。

    Computing the graph of Stations[0] = "岩見沢", Stations.size = 30, Edges.size = 36
    ---------- Best Result ----------
    # Total distance of all graph edges = 24577
    # Total distance of doubled edges = 11073
    # Total distance of traversed edges = 35650
    # Edges traversed twice in cuts
    # Edges traversed twice in component 1
    348 岩見沢 白石
    628 長万部 森
    765 桑園 新十津川
    （以下略）

のような表示が出るはずです。ここで

-   「Total distance of all graph edges」は、グラフ全体（jrhokkaido.edges）の辺の距離の総和です。
-   「Total distance of doubled edges」は、グラフの辺のうち、最短経路で全線を乗り尽くして起点駅に戻れるようにするために、2回通らないとならない辺の距離の総和です（これ以外の辺は1回ずつのみ通ります）。
-   「Total distance of traversed edges」は、グラフの辺のうち、最短経路で全線を乗り尽くして起点駅に戻るときの距離の総和です。すなわち、**「Total distance of all graph edges」と「Total distance of doubled edges」の和です**。

この場合、JR北海道には2457.7kmの路線があり、うち下に列挙された区間（合計1107.3km）のみを2度乗車して残りを1度ずつのみ乗車すれば、最短距離の乗車で全線を乗り尽くせることを意味しています。

### 3. 単純化してしてから解く

この方法では、駅数や辺数が比較的小さい路線網であったためにそのまま解けましたが、JR全線などを相手にすると流石に時間がかかりすぎます。そこで「路線網を分割してから解く」機構を用意しています。

まず以下のコマンドを実行します。

    mkdir jrhokkaido-div
    ./DivideByBridge.exe jrhokkaido.edges jrhokkaido-div

ここで「jrhokkaido」ディレクトリを見ると、3つのファイル「subgraph-森.edges」「subgraph-滝川.edges」「bridges.edges」が入っています（「森」と「滝川」は別の駅の名前になっている可能性もあります）。

これが意味するのは、1本の辺がなくなるだけで路線網が分断されるようなもの（これを「橋」という）を事前に集め、そこで路線網を分割することで解くべき問題を小さくしているのです。橋は「bridges.edges」に格納されます。  
なお、橋については二度通ることが確定する（仮に一度しか通れないと仮定すると、起点駅に戻ることができなくなる）ので、あとは残った各部分（この場合は「subgraph-森.edges」「subgraph-滝川.edges」）について距離最小の通り方を考えればよいということになります。

続いて以下のコマンドを実行します。

    ruby SolveChinesePostman.rb jrhokkaido-div

すると、計算経過ののちに以下のような表示が出るはずです。

    @ Total distance of bridge edges = 9191
    @ Total distance of doubled edges other than bridge = 1882
    @ Total distance of doubled edges = 11073

これが意味するのは、「橋であるために2度通ることが確定した辺」の距離の総和が9191、「橋以外で、最短乗車での乗り尽くしを達成するために2度通る辺」の距離の総和が1882であるというものです。

### 4. 任意の箇所で分割してから解く

これにより、路線網を橋によって分割して解けるようにはなりましたが、JRの本州のように橋が限定される（具体的には、行き止まりの区間でない限り橋にならない）地域では分割の効果が限定的になります。

そのため、任意の場所で分割を行えるようにする機構を用意してあります。

先ほどのjrhokkaido-divディレクトリにおいて、以下の操作をしてください。

1.  subgraph-滝川.edges（名前は異なる場合があるが、具体的には「森」でも「大沼」でもないほうのedges）をコピーし、「division-滝川.edges」のような名前にする（「subgraph」を「division」に置き換え、それ以外は変更しない）。
2.  division-滝川.edgesのうち、「546 滝川 富良野」「533 滝川 旭川」「1148 追分 新得」の3行だけ残して保存する。
3.  コマンド`ruby SolveChinesePostman.rb jrhokkaido-div`を実行する。

こうすると、結果として

    # ---------- Best Result ----------
    # Total distance of all graph edges = 14808
    # Total distance of doubled edges = 1882
    # Total distance of traversed edges = 16690
    # Edges traversed twice in cuts
    533 滝川 旭川
    # Edges traversed twice in component 1
    348 白石 岩見沢
    184 沼ノ端 南千歳
    # Edges traversed twice in component 2
    817 富良野 新得
    
    The result is written to "jrhokkaido-div/result.edges".
    @ Total distance of bridge edges = 9191
    @ Total distance of doubled edges other than bridge = 1882
    @ Total distance of doubled edges = 11073

のように表示されます。

これは路線網を「546 滝川 富良野」「533 滝川 旭川」「1148 追分 新得」の3つの辺によって分割し、その3辺それぞれを使う場合と使わない場合に場合分けして（この場合だと2の3乗 = 8通り）そのそれぞれについて最短の乗車経路を分割領域ごとに求める、ということを行っています。