// 全ページ共通の検索(外部通信なし)。search-index.js の索引を、関数名・クラス名・ファイル名・説明から探す。
(function () {
  var q = document.getElementById('q');
  var ul = document.getElementById('results');
  if (!q || !ul || !window.SEARCH_INDEX) return;

  function esc(s) {
    return String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  }

  function run() {
    var text = q.value.trim().toLowerCase();
    ul.innerHTML = '';
    if (text.length < 1) { ul.style.display = 'none'; return; }
    var hits = [];
    var data = window.SEARCH_INDEX;
    for (var i = 0; i < data.length; i++) {
      var e = data[i];
      var name = e.n.toLowerCase();
      var score = -1;
      if (name === text) score = 0;
      else if (name.indexOf(text) === 0) score = 1;
      else if (name.indexOf(text) >= 0) score = 2;
      else if (e.d.toLowerCase().indexOf(text) >= 0) score = 3;
      if (score >= 0) hits.push({ s: score, e: e });
    }
    hits.sort(function (a, b) { return a.s - b.s; });
    var max = Math.min(hits.length, 30);
    for (var j = 0; j < max; j++) {
      var h = hits[j].e;
      var li = document.createElement('li');
      var where = h.f ? ' <span class="where">(' + esc(h.f) + ')</span>' : '';
      li.innerHTML = '<a href="' + h.u + '"><span class="kind">' + esc(h.t) + '</span> <code>' + esc(h.n) + '</code>' + where + '</a>';
      ul.appendChild(li);
    }
    if (hits.length === 0) {
      var none = document.createElement('li');
      none.className = 'none';
      none.textContent = '見つかりませんでした';
      ul.appendChild(none);
    }
    ul.style.display = 'block';
  }

  q.addEventListener('input', run);
  q.addEventListener('focus', run);
  document.addEventListener('click', function (ev) {
    if (!ul.contains(ev.target) && ev.target !== q) ul.style.display = 'none';
  });
})();
