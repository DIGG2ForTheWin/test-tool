const body = document.querySelector("#machines tbody");
const password = document.getElementById("password");
const copy = document.getElementById("copy");

for (const m of LAB.machines) {
    const links = m.links.map((l, i) => `
        <li id="${m.name}-link-${i}" class="link down">
            <span class="dot"></span>
            <a href="${l.url}" target="_blank">${l.label}</a>
            <span class="offline">${l.label} - not reachable</span>
        </li>
    `).join("");

    body.innerHTML += `
        <tr id="${m.name}" class="machine">
            <td><button class="toggle" data-target="${m.name}-links" title="Show links">▸</button></td>
            <td>${m.name}</td>
            <td>${m.public_ip}</td>
            <td>${m.private_ip}</td>
            <td><span class="dot" id="${m.name}-status"></span></td>
        </tr>
        <tr id="${m.name}-links" class="links-row" hidden>
            <td></td>
            <td colspan="4">
                <ul class="links">${links || "<li>No web services on this machine</li>"}</ul>
            </td>
        </tr>`;
}

body.addEventListener("click", (event) => {
    const button = event.target.closest(".toggle");
    if (!button) return;
    const row = document.getElementById(button.dataset.target);
    row.hidden = !row.hidden;
    button.textContent = row.hidden? "▸" : "▾"
});
 

copy.addEventListener("click", () => {
    navigator.clipboard.writeText(password.textContent);
    copy.textContent = "Copied";
    setTimeout(() => {
        copy.textContent = "Copy";
    }, 2000);
});

async function isUp(url) {
  try {
    await fetch(url, { 
        mode: "no-cors",
        cache: "no-store" 
    });
    return true;            
  } catch {
    return false;         
  }
}

async function refreshStatus() {
  for (const m of LAB.machines) {
    const results = await Promise.all(m.links.map(l => isUp(l.url)));

    results.forEach((up, i) => {
        const li = document.getElementById(`${m.name}-link-${i}`);
        li.classList.toggle("up", up);
        li.classList.toggle("down", !up);
    });
    const dot = document.getElementById(`${m.name}-status`);
    dot.classList.toggle("up", results.some(Boolean));
  }
}

refreshStatus();
setInterval(refreshStatus, 5000);
