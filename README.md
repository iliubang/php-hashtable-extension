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

$ht->foreach(function($key, $val) {
    echo "key:" . $key . " === val:".$val."\n";
});

var_dump($ht->del("hello12"));
var_dump($ht->isset("hello13"));
```

Support Methods

```php
namespace linger;

class Hashtable {
    /**
     *@param int $size
     */
    public function __construct($size);
    
    /**
     *@param string $key
     *@param mixed $value
     *@return bool
     */
    public function set($key, $value);

    /**
     *@param string $key
     *@return mixed
     */
    public function get($key);

    /**
     *@param string $key
     *@return bool
     */
    public function isset($key);

    /**
     *@param string $key
     *@return bool
     */
    public function del($key);

    /**
     *@return int
     */
    public function getCount();

    /**
     *@return int
     */
    public function getSize();

    /**
     *@param callable $callback
     *@return bool
     */
    public function foreach($callback);

    public function __destruct();
}
```
