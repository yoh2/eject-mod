これは何?
=========

開くと光学ドライブのトレイがイジェクトされるデバイスファイル /dev/eject を
追加する Linux カーネルモジュールです。
使い途？ ジョーク以外に何かあるとでも？？

使い方
======

 1. ビルドします。

    make

 2. モジュールをロードします。モジュールパラメーター dev に光学ドライブのパスを
    指定できます。省略時は /dev/sr0 となります。

    sudo insmod eject.ko dev=/dev/cdrom

 3. /dev/eject を開くと光学ドライブのトレイが開きます。

    cat /dev/eject

補足
====

モジュールのインストール
------------------------

`make install` で、モジュールのインストールができ、 `depmod eject` でモジュールの
ロードができるようになります。

パーミッションエラーで開けない
------------------------------

カーネル設定によっては、/dev/eject は root 以外に開けないようになっている
場合があります。適切な udev のルールを /etc/udev/rules.d/ あたりに作ることで
一般ユーザーでも開けるようになります。

サンプルとして 99-eject.rules を同梱しています。これは、cdrom グループの
メンバーがファイルを開くことを許可するようになっています。

きっかけ
========

[こんなツイート](https://twitter.com/yoh2_sdj/status/754334349990907905) を
してしまったので勢いあまって実装してしまいました。

