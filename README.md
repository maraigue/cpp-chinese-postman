# Chinese Postman Problem Solver

## 問題設定について

[中国人郵便配達問題 - Wikipedia](https://ja.wikipedia.org/wiki/%E4%B8%AD%E5%9B%BD%E4%BA%BA%E9%83%B5%E4%BE%BF%E9%85%8D%E9%81%94%E5%95%8F%E9%A1%8C)

## 動かし方

### 0. 必要なもの

動かすまでの準備が多いです。また導入の敷居が高いものも多く使っています。改善したいところです。

-   （Windowsの場合）MinGW [http://www.mingw.org/](http://www.mingw.org/)  
    Unix系OSを想定したコードになっています。Windows環境においてはMinGWの利用を前提としています。
-   Boost C++ Libraries [http://www.boost.org/](http://www.boost.org/)  
    コンパイル済みのバージョンでなくても大丈夫です。Windowsの場合、ダウンロードして適当なフォルダに展開するだけでもよいです。
-   GLPK [https://www.gnu.org/software/glpk/](https://www.gnu.org/software/glpk/)  
    Windows環境の場合、上記MinGWを用いて、GLPKをソースコードからコンパイルしてください（注1）。"GLPK for Windows"での動作は未検証です。

（注1）これをどうすればよいか見当の付かない方には、環境構築全般も大変かと思います。参考までに、参考になるページをご紹介します。  
[「./configure」、「make」、「make install」の意味 - lovebug.jp](http://www.lovebug.jp/index.php?%E3%80%8C.%2Fconfigure%E3%80%8D%E3%80%81%E3%80%8Cmake%E3%80%8D%E3%80%81%E3%80%8Cmake%20install%E3%80%8D%E3%81%AE%E6%84%8F%E5%91%B3)

### 1. まずはビルドする

まず、Makefile内に記載されている`BOOST`と`GLPKDEVEL`のパスを書き換えます。`BOOST`はboostをインストールした先のディレクトリ、`GLPKDEVEL`はGLPKをインストールした先の**一つ上**のディレクトリ（例えば、`libglpk.a`が`/usr/local/lib`に入っているならば`GLPKDEVEL=/usr/local`）を指定します。

あとは

    make

コマンドでビルドが始まります。`DivideByBridge.exe`と`SolveChinesePostman.exe`が問題なく生成されれば成功です。

### 2. 普通に解く

