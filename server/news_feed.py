import time
import urllib.request
import urllib.parse
import xml.etree.ElementTree as ET
import html
import re
import json

from pathlib import Path

import requests
from bs4 import BeautifulSoup
from googlenewsdecoder import gnewsdecoder


BASE = Path.home() / "DSiMarketServer"

CACHE_FILE = BASE / "news_article_cache.json"

MAX_ARTICLE_CHARS = 2400


QUERIES = {
    "nas100":
        "Nasdaq 100 OR Nasdaq stocks OR technology stocks "
        "OR Nvidia OR Federal Reserve",

    "us30":
        "Dow Jones OR US stocks OR industrial stocks "
        "OR Federal Reserve",

    "gold":
        "gold price OR XAUUSD OR US dollar "
        "OR Treasury yields OR Federal Reserve",
}


HEADERS = {
    "User-Agent":
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
        "AppleWebKit/537.36 Safari/537.36"
}


# ==================================================
# CACHE
#
# We don't want to decode/download the same article
# every 15 minutes.
# ==================================================

def load_cache():

    if not CACHE_FILE.exists():
        return {}

    try:
        return json.loads(
            CACHE_FILE.read_text()
        )

    except Exception:
        return {}


def save_cache(cache):

    try:
        CACHE_FILE.write_text(
            json.dumps(cache)
        )

    except Exception:
        pass


cache = load_cache()


# ==================================================
# CLEAN TEXT
# ==================================================

def clean_text(text):

    if not text:
        return ""

    text = html.unescape(text)

    text = re.sub(
        r"<[^>]+>",
        " ",
        text
    )

    text = re.sub(
        r"\s+",
        " ",
        text
    )

    return text.strip()


# ==================================================
# GOOGLE NEWS LINK -> REAL PUBLISHER LINK
# ==================================================

def decode_news_url(google_url):

    try:

        result = gnewsdecoder(
            google_url,
            interval=0.5
        )

        if isinstance(result, dict):

            if result.get("status"):

                decoded = result.get(
                    "decoded_url"
                )

                if decoded:
                    return decoded

        elif isinstance(result, str):

            return result

    except Exception as error:

        print(
            "URL decode warning:",
            error
        )

    return ""


# ==================================================
# FIND ARTICLE BODY ON A PUBLIC PAGE
#
# We are NOT bypassing subscriptions/paywalls.
# If the public HTML doesn't contain readable text,
# we fall back to the RSS description.
# ==================================================

def extract_article_text(url):

    if not url:
        return ""

    try:

        response = requests.get(
            url,
            headers=HEADERS,
            timeout=12,
            allow_redirects=True
        )

        if response.status_code != 200:
            return ""

        soup = BeautifulSoup(
            response.text,
            "html.parser"
        )

        # Remove junk.
        for tag in soup(
            [
                "script",
                "style",
                "nav",
                "footer",
                "header",
                "aside",
                "form",
                "noscript"
            ]
        ):
            tag.decompose()

        # ==========================================
        # FIRST TRY JSON-LD articleBody
        # ==========================================

        for script in soup.find_all(
            "script",
            type="application/ld+json"
        ):

            try:

                raw = script.string

                if not raw:
                    continue

                data = json.loads(raw)

                objects = (
                    data
                    if isinstance(data, list)
                    else [data]
                )

                for obj in objects:

                    if not isinstance(
                        obj,
                        dict
                    ):
                        continue

                    body = obj.get(
                        "articleBody"
                    )

                    if body:

                        body = clean_text(
                            body
                        )

                        if len(body) > 150:

                            return body[
                                :MAX_ARTICLE_CHARS
                            ]

            except Exception:
                pass

        # ==========================================
        # THEN TRY ACTUAL ARTICLE PARAGRAPHS
        # ==========================================

        containers = []

        article_tag = soup.find(
            "article"
        )

        if article_tag:
            containers.append(
                article_tag
            )

        main_tag = soup.find(
            "main"
        )

        if main_tag:
            containers.append(
                main_tag
            )

        containers.append(
            soup
        )

        for container in containers:

            paragraphs = []

            seen = set()

            for p in container.find_all(
                "p"
            ):

                text = clean_text(
                    p.get_text(
                        " ",
                        strip=True
                    )
                )

                if len(text) < 45:
                    continue

                lowered = text.lower()

                junk_phrases = [
                    "sign up",
                    "subscribe",
                    "newsletter",
                    "all rights reserved",
                    "cookie policy",
                    "privacy policy",
                    "read more",
                    "advertisement"
                ]

                if any(
                    phrase in lowered
                    for phrase in junk_phrases
                ):
                    continue

                if text in seen:
                    continue

                seen.add(text)

                paragraphs.append(
                    text
                )

            body = " ".join(
                paragraphs
            )

            body = clean_text(
                body
            )

            if len(body) >= 180:

                return body[
                    :MAX_ARTICLE_CHARS
                ]

    except Exception as error:

        print(
            "Article extraction warning:",
            error
        )

    return ""


# ==================================================
# GET GOOGLE NEWS RSS
# ==================================================

def fetch_news(query):

    encoded = urllib.parse.quote(
        query
    )

    url = (
        "https://news.google.com/rss/search"
        f"?q={encoded}"
        "&hl=en-US"
        "&gl=US"
        "&ceid=US:en"
    )

    request = urllib.request.Request(
        url,
        headers=HEADERS
    )

    with urllib.request.urlopen(
        request,
        timeout=15
    ) as response:

        data = response.read()

    root = ET.fromstring(
        data
    )

    stories = []

    for item in root.findall(
        ".//item"
    )[:12]:

        title = clean_text(
            item.findtext(
                "title"
            )
        )

        rss_description = clean_text(
            item.findtext(
                "description"
            )
        )

        pub_date = clean_text(
            item.findtext(
                "pubDate"
            )
        )

        google_link = clean_text(
            item.findtext(
                "link"
            )
        )

        source_node = item.find(
            "source"
        )

        source = (
            clean_text(
                source_node.text
            )
            if source_node is not None
            else "NEWS"
        )

        # Remove publisher suffix from title.
        suffix = f" - {source}"

        if title.endswith(
            suffix
        ):
            title = title[
                :-len(suffix)
            ]

        # ==========================================
        # USE CACHE IF WE ALREADY FETCHED STORY
        # ==========================================

        body = cache.get(
            google_link,
            ""
        )

        if not body:

            print(
                "Fetching article:",
                title[:55]
            )

            real_url = decode_news_url(
                google_link
            )

            body = extract_article_text(
                real_url
            )

            # If publisher blocks extraction,
            # RSS remains our fallback.
            if len(body) < 120:

                body = rss_description

            if body:

                cache[
                    google_link
                ] = body

                save_cache(
                    cache
                )

        stories.append(
            {
                "title": title,
                "source": source,
                "date": pub_date,
                "body": body,
            }
        )

    return stories


# ==================================================
# DSi-FRIENDLY OUTPUT
# ==================================================

def safe_line(text):

    return (
        text
        .replace("|", "/")
        .replace("\n", " ")
        .replace("\r", " ")
        .strip()
    )


def save_market(
    name,
    stories
):

    path = BASE / (
        f"news_{name}.txt"
    )

    with open(
        path,
        "w",
        encoding="ascii",
        errors="ignore"
    ) as file:

        for story in stories:

            title = safe_line(
                story["title"]
            )

            source = safe_line(
                story["source"]
            )

            date = safe_line(
                story["date"]
            )

            body = safe_line(
                story["body"]
            )

            file.write(
                f"{title}|"
                f"{source}|"
                f"{date}|"
                f"{body}\n"
            )

    print(
        f"Updated {path.name}: "
        f"{len(stories)} stories"
    )


# ==================================================
# LOOP
# ==================================================

print(
    "INFINIT3 NEWS V2 feed starting..."
)

while True:

    for market, query in QUERIES.items():

        try:

            stories = fetch_news(
                query
            )

            save_market(
                market,
                stories
            )

        except Exception as error:

            print(
                f"{market.upper()} ERROR:",
                error
            )

    print(
        "------------------------------"
    )

    print(
        "Next news refresh in 15 minutes."
    )

    print()

    time.sleep(
        900
    )
