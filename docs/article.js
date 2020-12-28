(function (root, factory) {
  if (typeof define === 'function' && define.amd) {
    define(['exports'], factory);
  } else if (typeof exports === 'object' && typeof exports.nodeName !== 'string') {
    factory(exports);
  } else {
    factory(root.Article = { });
  }
} (this, function (exports) {
  function render (markdown) {
    return Promise.resolve(marked(markdown));
  }

  function article (url) {
    return fetch(url, {
      method: 'GET',
      mode: 'cors',
      cache: 'no-cache',
      headers: {
        'Content-Type': 'text/plain'
      }
    }).then(function (rsp) {
      return rsp.text();
    });
  }

  function attach (node, html) {
    if (typeof node == 'string')
      node = document.getElementById(node);
    node.innerHTML = html;

    return node;
  }

  function read (url, ref, nodes) {
    var hash = window.location.hash;

    return article(url)
      .then(function (markdown) {
        markdown = nodes.parse(markdown);

        return render(markdown)
          .then(function (html) {
            html = nodes.preprocess(html);

            attach(nodes.content, html);

            nodes.highlight();

            if (hash && hash.length > 0)
              window.location.hash = hash;
          });
      })
      .catch(function (_) {
        var html = 'Oops, cannot load content for the moment...<br>Try refersh or ';
        html += '<a href="' + ref + '" target="_blank">click</a>';

        attach(nodes.content, html);
      });
  }

  exports.render = render;
  exports.article = article;
  exports.attach = attach;
  exports.read = read;
}));
