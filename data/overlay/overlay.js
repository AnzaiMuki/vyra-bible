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
	var MIN_FONT_VH = 2.4;
	// Largest font of each theme, in vh of the screen.
	var MAX_FONT_VH = { lower: 5.2, full: 7.2, minimal: 5.2 };
	var DEFAULT_THEME = "lower";

	var panel = document.getElementById("panel");
	var slides = document.getElementById("slides");
	var lastRevision = -1;
	var currentKey = null; // what the visible slide shows, to skip a redraw of identical content
	var currentFrame = null; // the frame behind the visible slide, to lay it out again if the window is resized
	var currentTheme = null; // theme of the panel as drawn now
	var latestFrame = null; // the newest frame received (used when the panel waits for a theme change)
	var switching = false; // the panel is fading out to change theme

	function makeSlide(frame) {
		var slide = document.createElement("div");
		slide.className = "slide";
		var ref = document.createElement("div");
		ref.className = "ref";
		ref.appendChild(document.createTextNode(frame.reference));
		if (frame.pages > 1) {
			var page = document.createElement("span");
			page.className = "page";
			page.textContent = frame.page + "/" + frame.pages;
			ref.appendChild(page);
		}
		var text = document.createElement("div");
		text.className = "text";
		slide.appendChild(ref);
		slide.appendChild(text);
		return slide;
	}

	function fillText(textEl, verses, count, truncated, numbered) {
		textEl.textContent = "";
		for (var i = 0; i < count; i++) {
			// The piece of a verse that went over to this page ("k") has no number: it is not a new verse.
			if (numbered && !verses[i].k) {
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
	function fitFontSize(textEl, maxFont) {
		var lo = MIN_FONT_VH;
		var hi = maxFont;
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
		var numbered = frame.pages > 1 || verses.length > 1;
		var maxFont = MAX_FONT_VH[frame.theme] || MAX_FONT_VH[DEFAULT_THEME];
		slide.removeAttribute("data-truncated");
		fillText(textEl, verses, count, false, numbered);
		if (fitFontSize(textEl, maxFont) > 0) {
			return;
		}
		// Safety net (pages are cut to fit, so this should not happen): keep as many whole verses as fit.
		slide.setAttribute("data-truncated", "1");
		textEl.style.fontSize = MIN_FONT_VH + "vh";
		while (count > 1) {
			count--;
			fillText(textEl, verses, count, true, numbered);
			if (fits(textEl)) {
				return;
			}
		}
		fillText(textEl, verses, 1, true, numbered);
	}

	function setTheme(theme) {
		var name = MAX_FONT_VH[theme] ? theme : DEFAULT_THEME;
		panel.className = "theme-" + name;
		currentTheme = name;
	}

	function show(frame) {
		if (frame.rev <= lastRevision) {
			return; // an older frame arriving late
		}
		lastRevision = frame.rev;
		latestFrame = frame;
		if (switching) {
			return; // the newest frame is drawn when the panel has changed theme
		}

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

		var theme = MAX_FONT_VH[frame.theme] ? frame.theme : DEFAULT_THEME;
		if (currentTheme !== null && theme !== currentTheme) {
			// A change of theme moves the whole panel: it fades out, changes, and fades in again.
			if (panel.classList.contains("on")) {
				switching = true;
				panel.classList.remove("on");
				window.setTimeout(function () {
					switching = false;
					slides.textContent = "";
					currentKey = null;
					setTheme(latestFrame.theme);
					draw(latestFrame);
				}, FADE_MS + 30);
				return;
			}
		}
		if (currentTheme !== theme) {
			setTheme(theme);
		}
		draw(frame);
	}

	function draw(frame) {
		if (!frame.visible) {
			panel.classList.remove("on");
			currentKey = null;
			currentFrame = null;
			return;
		}
		var key = frame.theme + "\u0000" + frame.page + "/" + frame.pages + "\u0000" + frame.reference + "\u0000" +
			JSON.stringify(frame.verses);
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
