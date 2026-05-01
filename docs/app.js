const OWNER = "obsidian120";
const REPO = "Cyralith";

const latestReleaseUrl = `https://github.com/${OWNER}/${REPO}/releases/latest`;
const releasesUrl = `https://github.com/${OWNER}/${REPO}/releases`;
const sourceZipUrl = `https://github.com/${OWNER}/${REPO}/archive/refs/heads/main.zip`;
const apiUrl = `https://api.github.com/repos/${OWNER}/${REPO}/releases/latest`;

const primaryDownload = document.querySelector("#primaryDownload");
const downloadHint = document.querySelector("#downloadHint");
const releaseName = document.querySelector("#releaseName");
const releaseMeta = document.querySelector("#releaseMeta");
const assetList = document.querySelector("#assetList");

function formatBytes(bytes) {
  if (!Number.isFinite(bytes) || bytes <= 0) return "";
  const units = ["B", "KB", "MB", "GB"];
  const index = Math.min(Math.floor(Math.log(bytes) / Math.log(1024)), units.length - 1);
  const value = bytes / Math.pow(1024, index);
  return `${value.toFixed(value >= 10 || index === 0 ? 0 : 1)} ${units[index]}`;
}

function formatDate(dateString) {
  try {
    return new Intl.DateTimeFormat("de-DE", {
      year: "numeric",
      month: "long",
      day: "numeric"
    }).format(new Date(dateString));
  } catch {
    return dateString;
  }
}

function createAssetLink(asset) {
  const link = document.createElement("a");
  link.className = "asset-link";
  link.href = asset.browser_download_url;
  link.rel = "noreferrer";
  link.innerHTML = `
    <span>${asset.name}</span>
    <span class="asset-size">${formatBytes(asset.size)}</span>
  `;
  return link;
}

function choosePrimaryAsset(assets) {
  return (
    assets.find((asset) => /\.(iso|img|zip|7z|tar\.gz|bin)$/i.test(asset.name)) ||
    assets[0]
  );
}

async function loadLatestRelease() {
  try {
    const response = await fetch(apiUrl, {
      headers: { Accept: "application/vnd.github+json" }
    });

    if (!response.ok) {
      throw new Error(`GitHub API error: ${response.status}`);
    }

    const release = await response.json();
    const assets = Array.isArray(release.assets) ? release.assets : [];
    const primaryAsset = choosePrimaryAsset(assets);

    releaseName.textContent = release.name || release.tag_name || "Neueste Release";
    releaseMeta.textContent = [
      release.tag_name ? `Tag: ${release.tag_name}` : "",
      release.published_at ? `Veröffentlicht: ${formatDate(release.published_at)}` : ""
    ].filter(Boolean).join(" • ");

    assetList.innerHTML = "";

    if (assets.length > 0) {
      assets.forEach((asset) => assetList.appendChild(createAssetLink(asset)));

      primaryDownload.href = primaryAsset.browser_download_url;
      primaryDownload.textContent = `${primaryAsset.name} herunterladen`;
      downloadHint.textContent = `Direkter Download aus der neuesten GitHub Release.`;
    } else {
      const releaseLink = document.createElement("a");
      releaseLink.className = "asset-link";
      releaseLink.href = release.html_url || latestReleaseUrl;
      releaseLink.textContent = "Release-Seite öffnen";
      assetList.appendChild(releaseLink);

      const sourceLink = document.createElement("a");
      sourceLink.className = "asset-link";
      sourceLink.href = sourceZipUrl;
      sourceLink.textContent = "Quellcode als ZIP";
      assetList.appendChild(sourceLink);

      primaryDownload.href = release.html_url || latestReleaseUrl;
      primaryDownload.textContent = "Neueste Release öffnen";
      downloadHint.textContent = "Diese Release enthält keine erkannten Binärdateien. Öffne die Release-Seite oder lade den Quellcode.";
    }
  } catch (error) {
    releaseName.textContent = "Release konnte nicht automatisch geladen werden";
    releaseMeta.textContent = "Nutze die Fallback-Links zu GitHub.";
    assetList.innerHTML = "";

    const releaseLink = document.createElement("a");
    releaseLink.className = "asset-link";
    releaseLink.href = latestReleaseUrl;
    releaseLink.textContent = "Neueste Release öffnen";
    assetList.appendChild(releaseLink);

    const allReleasesLink = document.createElement("a");
    allReleasesLink.className = "asset-link";
    allReleasesLink.href = releasesUrl;
    allReleasesLink.textContent = "Alle Releases";
    assetList.appendChild(allReleasesLink);

    const sourceLink = document.createElement("a");
    sourceLink.className = "asset-link";
    sourceLink.href = sourceZipUrl;
    sourceLink.textContent = "Quellcode als ZIP";
    assetList.appendChild(sourceLink);

    primaryDownload.href = latestReleaseUrl;
    primaryDownload.textContent = "Neueste Release öffnen";
    downloadHint.textContent = "Fallback aktiv. GitHub API kann z. B. wegen Rate-Limits blockieren.";
  }
}

loadLatestRelease();
