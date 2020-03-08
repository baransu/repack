
(function(modules) {
  function require(id) {
    const [fn, mapping] = modules[id];
    function localRequire(name) {
      return require(mapping[name]);
    }
    const module = { exports : {} };
    fn(localRequire, module, module.exports);
    return module.exports;
  }
  require(0);
  })({0: [function (require, module, exports) {const a = require("./a");
const b = require("./b");

console.log(a.hello(b.getName()));
},{"./a":2,"./b":1}],1: [function (require, module, exports) {function getName() {
  return "Jordan";
}

exports.getName = getName;
},{}],2: [function (require, module, exports) {function hello(name) {
  return `Hello ${name}`;
}

exports.hello = hello;
},{}]})
