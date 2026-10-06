/*
 * VYRA Bible overlay. Receives complete frames from the plugin (Server-Sent Events) and draws them.
 * Rules:
 *  - a new verse cross-fades over the old one: the panel never goes empty between two verses;
 *  - the text is made to fit the panel; if it still does not fit at the smallest size, whole verses are
 *    dropped from the end and "…" is added (never a half-cut line);
 *  - if the connection to the plugin drops, the last picture stays (a live screen must not go blank
 *    because of a network hiccup); the browser reconnects by itself.
 */
(function () {
	"use strict";

	var FADE_MS = 300;
	var MAX_FONT_VH = 5.2;
	var MIN_FONT_VH = 2.4;

	var panel = document.getElementById("panel");
	var slides = document.getElementById("slides");
	var lastRevision = -1;
	var currentKey = null; // what the visible slide shows, to skip a redraw of identical content
	var currentFrame = null; // the frame behind the visible slide, to lay it out again if the window is resized

	function makeSlide(frame) {
		var slide = document.createElement("div");
		slide.className = "slide";
		var ref = document.createElement("div");
		ref.className = "ref";
		ref.textContent = frame.reference;
		var text = document.createElement("div");
		text.className = "text";
		slide.appendChild(ref);
		slide.appendChild(text);
		return slide;
	}

	function fillText(textEl, verses, count, truncated) {
		textEl.textContent = "";
		var numbered = verses.length > 1;
		for (var i = 0; i < count; i++) {
			if (numbered) {
				var sup = document.createElement("sup");
				sup.textContent = String(verses[i].v);
				textEl.appendChild(sup);
			}
			textEl.appendChild(document.createTextNode(verses[i].t + (i < count - 1 ? " " : "")));
		}
		if (truncated) {
			textEl.appendChild(document.createTextNode(" …"));
		}
	}

	function fits(textEl) {
		return textEl.scrollHeight <= textEl.clientHeight + 1;
	}

	// The largest font size (in vh) at which the text fits, or 0 if even the smallest does not.
	function fitFontSize(textEl) {
		var lo = MIN_FONT_VH;
		var hi = MAX_FONT_VH;
		textEl.style.fontSize = lo + "vh";
		if (!fits(textEl)) {
			return 0;
		}
		for (var i = 0; i < 12; i++) {
			var mid = (lo + hi) / 2;
			textEl.style.fontSize = mid + "vh";
			if (fits(textEl)) {
				lo = mid;
			} else {
				hi = mid;
			}
		}
		textEl.style.fontSize = lo + "vh";
		return lo;
	}

	function layoutSlide(slide, frame) {
		var textEl = slide.querySelector(".text");
		var verses = frame.verses;
		var count = verses.length;
		fillText(textEl, verses, count, false);
		if (fitFontSize(textEl) > 0) {
			return;
		}
		// Too long even at the smallest size: keep as many whole verses as fit.
		textEl.style.fontSize = MIN_FONT_VH + "vh";
		while (count > 1) {
			count--;
			fillText(textEl, verses, count, true);
			if (fits(textEl)) {
				return;
			}
		}
		fillText(textEl, verses, 1, true);
	}

	function show(frame) {
		if (frame.rev <= lastRevision) {
			return; // an older frame arriving late
		}
		lastRevision = frame.rev;

		if (!frame.visible) {
			currentKey = null;
			currentFrame = null;
			panel.classList.remove("on");
			// Empty the panel once it has faded out, unless something new was shown meanwhile.
			var revisionAtHide = frame.rev;
			window.setTimeout(function () {
				if (lastRevision === revisionAtHide) {
					slides.textContent = "";
				}
			}, FADE_MS + 50);
			return;
		}

		var key = frame.reference + "\u0000" + JSON.stringify(frame.verses);
		if (key === currentKey) {
			panel.classList.add("on");
			return;
		}
		currentKey = key;
		currentFrame = frame;

		var slide = makeSlide(frame);
		slide.classList.add("measuring");
		slides.appendChild(slide);
		layoutSlide(slide, frame);
		slide.classList.remove("measuring");

		var old = Array.prototype.slice.call(slides.children).filter(function (el) {
			return el !== slide;
		});
		// Two frames later so that the opacity transition starts from 0.
		window.requestAnimationFrame(function () {
			window.requestAnimationFrame(function () {
				slide.classList.add("shown");
				old.forEach(function (el) {
					el.classList.remove("shown");
				});
				panel.classList.add("on");
				window.setTimeout(function () {
					old.forEach(function (el) {
						if (el.parentNode) {
							el.parentNode.removeChild(el);
						}
					});
				}, FADE_MS + 50);
			});
		});
	}

	// OBS can resize a Browser Source while it is on the air: the text is fitted again, without any fade.
	var resizeTimer = null;
	window.addEventListener("resize", function () {
		window.clearTimeout(resizeTimer);
		resizeTimer = window.setTimeout(function () {
			var shown = slides.querySelector(".slide.shown");
			if (shown && currentFrame) {
				layoutSlide(shown, currentFrame);
			}
		}, 80);
	});

	function connect() {
		var source = new EventSource("events");
		source.onopen = function () {
			lastRevision = -1; // the plugin may have restarted: its revisions start again from 1
		};
		source.onmessage = function (event) {
			try {
				show(JSON.parse(event.data));
			} catch (e) {
				if (window.console) {
					console.error("VYRA overlay: bad frame", e);
				}
			}
		};
		// On error the browser reconnects by itself; the last picture stays.
	}

	// Test hook: lets a page without a plugin feed frames by hand (used by the automated tests).
	window.vyraShow = show;

	if (!/[?&]manual=1/.test(window.location.search) && window.EventSource) {
		connect();
	}
})();
