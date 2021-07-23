# Silvermine UnderTemplate

[![Build Status](https://travis-ci.org/silvermine/undertemplate.svg?branch=master)](https://travis-ci.org/silvermine/undertemplate)
[![Coverage Status](https://coveralls.io/repos/github/silvermine/undertemplate/badge.svg?branch=master)](https://coveralls.io/github/silvermine/undertemplate?branch=master)
[![Dependency Status](https://david-dm.org/silvermine/undertemplate.svg)](https://david-dm.org/silvermine/undertemplate)
[![Dev Dependency Status](https://david-dm.org/silvermine/undertemplate/dev-status.svg)](https://david-dm.org/silvermine/undertemplate#info=devDependencies&view=table)


## What is it?

A simple replacement for `_.template` from either [Underscore][utmpl] or [Lodash][ltmpl]
that removes the features that make those libraries incompatible with Content Security
Policy (CSP). Specifically, this implementation supports only interpolation (escaped and
unescaped) and does not support JS evaluation.


## How do I use it?

Just like you would have used `_.template`:

```
var makeTemplate = require('@silvermine/undertemplate'),
    template = makeTemplate('Hello <%= name %>');

console.log(template({ name: 'John Smith' }));
```

Of course, we only support:

   * `<%= … %>`: interpolate a value
   * `<%- … %>`: interpolate and HTML escape a value

The following are **NOT** supported:

   * `<% … %>`: JS evaluation
   * `<% print('Hello ' + epithet); %>`: JS evaluation with the `print` function


## A Couple Notes

Templating in Underscore/Lodash was operating by building up a JS function as a string.
This meant that if your template referred to a variable that did not exist, you would get
a JS error thrown. In this library, however, undefined variables in the template will
result in an empty string being placed in that location.

We rely on Lodash to provide a number of convenience functions that would require
polyfills to support a wide array of browsers. To help reduce bloat, we do two things:

   1. Rely on a fairly loose version of Lodash: `4.x`, meaning that if you already have it
      as a dependency, we'll use your version.
   2. Only `require('lodash/foo')` for each `foo` function we need. This helps if you're
      using UnderTemplate in a browserify-style environment since only the files from
      Lodash that are actually needed will be included in your bundle.


## How do I contribute?

We genuinely appreciate external contributions. See [our extensive
documentation][contributing] on how to contribute.


## License

This software is released under the MIT license. See [the license file](LICENSE) for more
details.

[contributing]: https://github.com/silvermine/silvermine-info#contributing
[utmpl]: http://underscorejs.org/#template
[ltmpl]: https://lodash.com/docs/#template
