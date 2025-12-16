(() => {
    // ===== 設定 =====
    const deviceId = "demo-01";
    const apiBase = location.origin;

    // 効果音（public/success.mp3 を置いた前提）
    const SOUND_URL = "/success.mp3";

    // ===== DOM =====
    const card = document.getElementById("card");
    const totalEl = document.getElementById("total");
    const amountInput = document.getElementById("amount");
    const btnAdd = document.getElementById("btnAdd");
    const btnReset = document.getElementById("btnReset");
    const btnRefresh = document.getElementById("btnRefresh");
    const messageEl = document.getElementById("message");
    const netPill = document.getElementById("netPill");
    const busyPill = document.getElementById("busyPill");

    let busy = false;

    function setMsg(text, ok = true) {
        messageEl.textContent = text || "";
        messageEl.className = ok ? "ok" : "ng";
    }

    function setBusy(v) {
        busy = v;
        btnAdd.disabled = v;
        btnReset.disabled = v;
        btnRefresh.disabled = v;
        busyPill.style.visibility = v ? "visible" : "hidden";
    }

    function setOnlinePill() {
        const on = navigator.onLine;
        netPill.textContent = on ? "online" : "offline";
        netPill.className = on ? "pill" : "pill offline";
        if (!on) setMsg("オフラインです。通信できません。", false);
    }

    function flash() {
        card.classList.remove("flash");
        // 再トリガー用
        void card.offsetWidth;
        card.classList.add("flash");
    }

    function shake() {
        card.classList.remove("shake");
        void card.offsetWidth;
        card.classList.add("shake");
    }

    // ===== 効果音（iOS対策：ユーザー操作でアンロック）=====
    const successSound = new Audio(SOUND_URL);
    successSound.preload = "auto";
    successSound.volume = 0.35;

    let audioUnlocked = false;

    async function unlockAudioOnce() {
        if (audioUnlocked) return;
        try {
            // iOSは「ユーザー操作中に play()」した実績が必要なことが多いので、
            // 0秒再生→停止でアンロックを狙う（失敗してもOK）
            successSound.muted = true;
            await successSound.play();
            successSound.pause();
            successSound.currentTime = 0;
            successSound.muted = false;
            audioUnlocked = true;
        } catch (_) {
            // ここで失敗しても、次のユーザー操作で鳴る可能性があります
        }
    }

    function playSuccessSound() {
        try {
            // iOS: クリック直後でも、currentTime リセットが必要なことがある
            successSound.currentTime = 0;
            const p = successSound.play();
            if (p && typeof p.catch === "function") p.catch(() => { });
        } catch (_) { }
    }

    async function fetchJson(url, options) {
        const res = await fetch(url, options);
        const text = await res.text();
        if (!res.ok) throw new Error(`HTTP ${res.status}: ${text}`);
        return text ? JSON.parse(text) : {};
    }

    async function loadTotal() {
        if (busy) return;
        if (!navigator.onLine) { setOnlinePill(); return; }

        setBusy(true);
        setMsg("取得中…", true);

        try {
            const data = await fetchJson(`${apiBase}/api/device?deviceId=${encodeURIComponent(deviceId)}`, {
                method: "GET",
                headers: { "Accept": "application/json" }
            });

            if (typeof data.total === "number") {
                totalEl.textContent = String(data.total);
                setMsg("更新しました。", true);
            } else {
                setMsg("取得できましたが形式が想定外です:\n" + JSON.stringify(data, null, 2), false);
            }
        } catch (err) {
            console.error(err);
            setMsg("取得エラー:\n" + (err?.message || String(err)), false);
            shake();
        } finally {
            setBusy(false);
        }
    }

    async function readAmountStable() {
        // スマホで入力確定させる
        amountInput.blur();
        await new Promise(r => setTimeout(r, 0));

        const n = amountInput.valueAsNumber;
        if (Number.isFinite(n)) return n;
        return Number((amountInput.value || "").trim());
    }

    async function addSavings() {
        if (busy) return;
        if (!navigator.onLine) { setOnlinePill(); return; }

        // iOS対策：最初のボタン操作で音をアンロック
        await unlockAudioOnce();

        const amount = await readAmountStable();
        if (!Number.isFinite(amount) || amount <= 0) {
            setMsg("金額を正しく入力してください。", false);
            shake();
            return;
        }

        setBusy(true);
        setMsg("送信中…", true);

        // 見た目だけ先に加算（失敗したら戻す）
        const prevTotal = Number(totalEl.textContent) || 0;
        totalEl.textContent = String(prevTotal + amount);

        try {
            const data = await fetchJson(`${apiBase}/api/savings`, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                    "Accept": "application/json"
                },
                body: JSON.stringify({ amount: Number(amount), deviceId })
            });

            if (typeof data.total === "number") {
                totalEl.textContent = String(data.total);
                amountInput.value = "";

                setMsg(`+${amount} 円 入金しました。`, true);
                flash();
                playSuccessSound();
            } else {
                totalEl.textContent = String(prevTotal);
                setMsg("送信できましたが形式が想定外です:\n" + JSON.stringify(data, null, 2), false);
                shake();
            }
        } catch (err) {
            console.error(err);
            totalEl.textContent = String(prevTotal);
            setMsg("送信エラー:\n" + (err?.message || String(err)), false);
            shake();
        } finally {
            setBusy(false);
        }
    }

    async function resetAll() {
        if (busy) return;
        if (!navigator.onLine) { setOnlinePill(); return; }

        // iOS対策：リセットでもアンロックを試す
        await unlockAudioOnce();

        if (!confirm("本当にリセットしますか？")) return;

        setBusy(true);
        setMsg("リセット中…", true);

        const prevTotal = Number(totalEl.textContent) || 0;

        try {
            const data = await fetchJson(`${apiBase}/api/reset`, {
                method: "POST",
                headers: { "Accept": "application/json" }
            });

            if (typeof data.total === "number") {
                totalEl.textContent = String(data.total);
                setMsg("リセットしました。", true);
                flash();
                playSuccessSound(); // リセットも音鳴らす（不要なら削除OK）
            } else {
                totalEl.textContent = String(prevTotal);
                setMsg("リセットできましたが形式が想定外です:\n" + JSON.stringify(data, null, 2), false);
                shake();
            }
        } catch (err) {
            console.error(err);
            totalEl.textContent = String(prevTotal);
            setMsg("リセットエラー:\n" + (err?.message || String(err)), false);
            shake();
        } finally {
            setBusy(false);
        }
    }

    // ===== イベント =====
    // iOSは「ユーザー操作で音のアンロック」が必要なので、どのボタンでも一度試す
    const unlockHandler = () => unlockAudioOnce();
    document.addEventListener("pointerdown", unlockHandler, { passive: true, once: true });
    document.addEventListener("touchstart", unlockHandler, { passive: true, once: true });

    btnAdd.addEventListener("click", (e) => { e.preventDefault(); addSavings(); });
    btnAdd.addEventListener("pointerup", (e) => { e.preventDefault(); addSavings(); }); // スマホ安定

    btnRefresh.addEventListener("click", (e) => { e.preventDefault(); loadTotal(); });
    btnReset.addEventListener("click", (e) => { e.preventDefault(); resetAll(); });

    amountInput.addEventListener("keydown", (e) => {
        if (e.key === "Enter") {
            e.preventDefault();
            addSavings();
        }
    });

    window.addEventListener("online", setOnlinePill);
    window.addEventListener("offline", setOnlinePill);

    // 起動時
    setOnlinePill();
    loadTotal();
})();
