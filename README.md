これは何?
=========

開くと光学ドライブのトレイがイジェクトされるデバイスファイル /dev/eject と
閉じるとトレイが戻るところまでが組の /dev/cdtray を追加する Linux カーネル
モジュールです。
使い途？ ジョーク以外に何かあるとでも？？

使い方
======

 1. ビルドします。

    ```
    make
    ```

 2. モジュールをロードします。モジュールパラメーター dev に光学ドライブのパスを
    指定できます。省略時は /dev/sr0 となります。

    ```
    sudo insmod eject.ko dev=/dev/cdrom
    ```

 3. /dev/eject を開くと光学ドライブのトレイが開きます。/dev/cdtray ならば、
    ファイルが閉じるタイミングでトレイが閉じます。

    ```
    # トレイが開く (開きっぱなし)
    cat /dev/eject
    
    # トレイが開いた後閉じる
    cat /dev/cdtray
    ```

補足
====

モジュールのインストール
------------------------

`make install` で、モジュールのインストールができ、 `modprobe eject` でモジュールの
ロードができるようになります。

パーミッションエラーで開けない
------------------------------

システム設定によっては、/dev/eject は root 以外に開けないようになっている
場合があります。適切な udev のルールを /etc/udev/rules.d/ あたりに作ることで
一般ユーザーでも開けるようになります。

サンプルとして 99-eject.rules を同梱しています。これは、cdrom グループの
メンバーがファイルを開くことを許可するようになっています。

モジュールをロードしてから対象デバイスを変更する
------------------------------------------------

/sys/module/eject/parameters/dev を読み書き可能パラメータとしてありますので、
ここを書き換えることでアンロードせずに対象を変更することができます。
パーミッションは 0600 としているのでスーパーユーザー権限が必要です。

```
    sudo echo -n /dev/sr1 > /sys/module/eject/parameters/dev
```

`echo` の `-n` 引数を忘れると、改行までファイル名に含まれてしまいますので注意。
(この辺対応しつつ設定を proc に移すべきか……)

きっかけ
========

[こんなツイート](https://twitter.com/yoh2_sdj/status/754334349990907905) を
してしまったので勢いあまって実装してしまいました。

