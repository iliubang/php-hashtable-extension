# a simple hashtable implementation

## Require

- php >= 5.3

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
$ht = new Linger\Hashtable();
$ht->set("hello", "world");
$ht['name'] = 'liubang';
$ht['email'] = 'it.liubang@gmail.com';
var_dump($ht->isset('name'));
var_dump($ht->get('name'));
var_dump($ht['name']);

$ht->foreach(function($key, $val) {
    echo $key, "===", $val, "\n";
});

foreach ($ht as $key => $val) {
    echo $key, "===", $val, "\n";
}

echo $ht->count();
echo count($ht);
```

## Feature

- ArrayAccess
- Traversable

## Benchmark

```php
<?php

$ht = new linger\Hashtable();

$n = 10000;

$start = microtime(true);
for ($i = 0; $i < $n; $i++) {
    $ht["hello{$i}"] = "world{$i}";

}

for ($i = 0; $i < $n; $i++) {
    $ht["hello{$i}"];

}
echo "HashTable:" . (microtime(true) - $start), PHP_EOL;

$arr = [];
$start = microtime(true);
for ($i = 0; $i < $n; $i++) {
   $arr["hello{$i}"] = "world{$i}"; 

}

for ($i = 0; $i < $n; $i++) {
    $arr["hello{$i}"];

}
echo "    Array:" . (microtime(true) - $start), PHP_EOL;
```
result

```
liubang@venux:~/workspace/c/php-hashtable-extension/tests$ php test2.php 
HashTable:0.0060431957244873
    Array:0.0054750442504883
```

## methods

```php
/**
 * @param string $key
 * @param mixed $value
 * @return bool
 */
public function set($key, $value);

/**
 * @param string $key
 * @return mixed
 */
public function get($key);

/**
 * @param string $key
 * @return bool
 */
public function isset($key);

/**
 * @param string $key
 * @return bool
 */
public function del($key);

/**
 * @return int
 */
public function count();

/**
 * @param callable $callback
 * @return bool
 */
public function foreach($callback);

```
