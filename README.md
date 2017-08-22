# a simple hashtable implement

## Require
- php <= 5.6

```
git clone https://github.com/iliubang/php-hashtable-extension.git
cd php-hashtable-extension
phpize
./configure
make && sudo make install

echo 'extension = linger_hashtable.so' >> {your php ini path}/php-cli.ini
```

```php
<?php

$ht = new Linger\Hashtable(6553500);
var_dump($ht);
$ht->set("hello", "world");
var_dump($ht->get("hello"));

$n = 10000;

$start = microtime(true);

for($i = 0; $i < $n; $i++) {
	$ht->set("hello".$i, "world".$i);
}

for($i = 0; $i < $n; $i++) {
	$tmp = $ht->get("hello".$i);
}

echo microtime(true) - $start,PHP_EOL;

var_dump($ht->getSize());
var_dump($ht->getCount());

$start = microtime(true);

for ($i = 0; $i < $n; $i++) {
	$arr["hello".$i] = "world".$i;
}

for ($i = 0; $i < $n; $i++) {
	$tmp = $arr["hello".$i];
}

echo microtime(true) - $start,PHP_EOL;

for ($i = 0; $i < $n; $i++) {
    $res = $ht->del("hello".$i);
    if (!$res) {
        echo "fail:"."hello".$i,PHP_EOL;
    }
}

var_dump($ht->getCount());
```
