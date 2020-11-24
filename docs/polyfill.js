if (!String.prototype.replaceAll) {
  Object.defineProperty(String.prototype, 'replaceAll', {
    value: function (search, replace) {
      return this.split(search).join(replace);
    }
  });
}
