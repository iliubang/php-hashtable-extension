<?php

$ht = new linger\Hashtable(655350);

$ht->set("func", function() {
    echo "hello world\n";
});

var_dump($ht->get("func"));
$func = $ht->get("func");
$func();


class Foo {
    private $name;
    private $age;
    public function __construct($name, $age) {
        $this->name = $name;
        $this->age = $age;
    }

    public function getName() {
        return $this->name;
    } 

    public function getAge() {
        return $this->age;
    }
}


$foo = new Foo("liubang", 23);

$ht->set("obj", $foo);
var_dump($ht->get("obj"));

$obj = $ht->get("obj");
var_dump($obj->getName());
var_dump($obj->getAge());
