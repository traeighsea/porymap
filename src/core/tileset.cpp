#include "tileset.h"
#include "metatile.h"
#include "project.h"
#include "log.h"
#include "config.h"
#include "imageproviders.h"
#include "validator.h"
#include "utility.h"
#include "parseutil.h"

#include <QPainter>
#include <QImage>
#include <algorithm>


Tileset::Tileset(const Tileset &other)
    : name(other.name),
      is_secondary(other.is_secondary),
      tiles_label(other.tiles_label),
      palettes_label(other.palettes_label),
      metatiles_label(other.metatiles_label),
      metatiles_path(other.metatiles_path),
      metatile_attrs_label(other.metatile_attrs_label),
      metatile_attrs_path(other.metatile_attrs_path),
      tilesImagePath(other.tilesImagePath),
      palettePaths(other.palettePaths),
      metatileLabels(other.metatileLabels),
      palettes(other.palettes),
      palettePreviews(other.palettePreviews),
      m_tilesImage(other.m_tilesImage.copy()),
      m_hasUnsavedTilesImage(other.m_hasUnsavedTilesImage),
      m_metatileAttrBitMasks(other.m_metatileAttrBitMasks),
      m_numTiles(other.m_numTiles),
      m_numPals(other.m_numPals)
{
    for (auto tile : other.m_tiles) {
        m_tiles.append(tile.copy());
    }

    for (auto *metatile : other.m_metatiles) {
        m_metatiles.append(new Metatile(*metatile));
    }
}

Tileset &Tileset::operator=(const Tileset &other) {
    name = other.name;
    is_secondary = other.is_secondary;
    tiles_label = other.tiles_label;
    palettes_label = other.palettes_label;
    metatiles_label = other.metatiles_label;
    metatiles_path = other.metatiles_path;
    metatile_attrs_label = other.metatile_attrs_label;
    metatile_attrs_path = other.metatile_attrs_path;
    tilesImagePath = other.tilesImagePath;
    m_tilesImage = other.m_tilesImage.copy();
    palettePaths = other.palettePaths;
    metatileLabels = other.metatileLabels;
    palettes = other.palettes;
    palettePreviews = other.palettePreviews;
    m_metatileAttrBitMasks = other.m_metatileAttrBitMasks;
    m_numMetatiles = other.m_numMetatiles;
    m_numTiles = other.m_numTiles;
    m_numPals = other.m_numPals;

    m_tiles.clear();
    for (auto tile : other.m_tiles) {
        m_tiles.append(tile.copy());
    }

    clearMetatiles();
    for (auto *metatile : other.m_metatiles) {
        m_metatiles.append(new Metatile(*metatile));
    }

    return *this;
}

Tileset::~Tileset() {
    clearMetatiles();
}

void Tileset::clearMetatiles() {
    qDeleteAll(m_metatiles);
    m_metatiles.clear();
}

void Tileset::setMetatiles(const QList<Metatile*> &metatiles) {
    clearMetatiles();
    m_metatiles = metatiles;
}

void Tileset::addMetatile(Metatile* metatile) {
    m_metatiles.append(metatile);
}

void Tileset::resizeMetatiles(int newNumMetatiles) {
    if (newNumMetatiles < 0) newNumMetatiles = 0;
    while (m_metatiles.length() > newNumMetatiles) {
        delete m_metatiles.takeLast();
    }
    const int numTiles = projectConfig.getNumTilesInMetatile();
    while (m_metatiles.length() < newNumMetatiles) {
        m_metatiles.append(new Metatile(numTiles));
    }
}
uint16_t Tileset::firstMetatileId() const {
    return this->is_secondary ? Project::getNumMetatilesPrimary() : 0;
}

uint16_t Tileset::lastMetatileId() const {
    return qMax(1, firstMetatileId() + m_metatiles.length()) - 1;
}

int Tileset::numMetatiles() const {
    return m_numMetatiles.value_or(m_metatiles.length());
}

int Tileset::maxMetatiles() const {
    return m_numMetatiles.value_or(this->is_secondary ? Project::getNumMetatilesSecondary() : Project::getNumMetatilesPrimary());
}

uint16_t Tileset::firstTileId() const {
    return this->is_secondary ? Project::getNumTilesPrimary() : 0;
}

uint16_t Tileset::lastTileId() const {
    return qMax(1, firstMetatileId() + m_tiles.length()) - 1;
}

int Tileset::numTiles() const {
    return m_numTiles.value_or(m_tiles.length());
}

int Tileset::maxTiles() const {
    return m_numTiles.value_or(is_secondary ? Project::getNumTilesTotal() - Project::getNumTilesPrimary() : Project::getNumTilesPrimary());
}

Tileset* Tileset::getPaletteTileset(int paletteId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    return const_cast<Tileset*>(getPaletteTileset(paletteId, static_cast<const Tileset*>(primaryTileset), static_cast<const Tileset*>(secondaryTileset)));
}

const Tileset* Tileset::getPaletteTileset(int paletteId, const Tileset *primaryTileset, const Tileset *secondaryTileset) {
    if (paletteId < Project::getNumPalettesPrimary()) {
        return primaryTileset;
    } else if (paletteId < Project::getNumPalettesTotal()) {
        return secondaryTileset;
    } else {
        return nullptr;
    }
}

Tileset* Tileset::getTileTileset(int tileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    return const_cast<Tileset*>(getTileTileset(tileId, static_cast<const Tileset*>(primaryTileset), static_cast<const Tileset*>(secondaryTileset)));
}

// Get the tileset *expected* to contain the given 'tileId'. Note that this does not mean the tile actually exists in that tileset.
const Tileset* Tileset::getTileTileset(int tileId, const Tileset *primaryTileset, const Tileset *secondaryTileset) {
    if (tileId < Project::getNumTilesPrimary()) {
        return primaryTileset;
    } else if (tileId < Project::getNumTilesTotal()) {
        return secondaryTileset;
    } else {
        return nullptr;
    }
}

Tileset* Tileset::getMetatileTileset(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    return const_cast<Tileset*>(getMetatileTileset(metatileId, static_cast<const Tileset*>(primaryTileset), static_cast<const Tileset*>(secondaryTileset)));
}

// Get the tileset *expected* to contain the given 'metatileId'. Note that this does not mean the metatile actually exists in that tileset.
const Tileset* Tileset::getMetatileTileset(int metatileId, const Tileset *primaryTileset, const Tileset *secondaryTileset) {
    if (metatileId < Project::getNumMetatilesPrimary()) {
        return primaryTileset;
    } else if (metatileId < Project::getNumMetatilesTotal()) {
        return secondaryTileset;
    } else {
        return nullptr;
    }
}

Metatile* Tileset::getMetatile(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    return const_cast<Metatile*>(getMetatile(metatileId, static_cast<const Tileset*>(primaryTileset), static_cast<const Tileset*>(secondaryTileset)));
}

const Metatile* Tileset::getMetatile(int metatileId, const Tileset *primaryTileset, const Tileset *secondaryTileset) {
    const Tileset *tileset = Tileset::getMetatileTileset(metatileId, primaryTileset, secondaryTileset);
    if (!tileset) {
        return nullptr;
    }
    int index = Metatile::getIndexInTileset(metatileId);
    return tileset->m_metatiles.value(index, nullptr);
}

// Metatile labels are stored per-tileset. When looking for a metatile label, first search in the tileset
// that the metatile belongs to. If one isn't found, search in the other tileset. Labels coming from the
// tileset that the metatile does not belong to are shared and cannot be edited via Porymap.
Tileset* Tileset::getMetatileLabelTileset(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    Tileset *mainTileset = nullptr;
    Tileset *alternateTileset = nullptr;
    if (metatileId < Project::getNumMetatilesPrimary()) {
        mainTileset = primaryTileset;
        alternateTileset = secondaryTileset;
    } else if (metatileId < Project::getNumMetatilesTotal()) {
        mainTileset = secondaryTileset;
        alternateTileset = primaryTileset;
    }

    if (mainTileset && !mainTileset->metatileLabels.value(metatileId).isEmpty()) {
        return mainTileset;
    } else if (alternateTileset && !alternateTileset->metatileLabels.value(metatileId).isEmpty()) {
        return alternateTileset;
    }
    return nullptr;
}

// Return the pair of possible metatile labels for the specified metatile.
// "owned" is the label for the tileset to which the metatile belongs.
// "shared" is the label for the tileset to which the metatile does not belong.
MetatileLabelPair Tileset::getMetatileLabelPair(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    MetatileLabelPair labels;
    QString primaryMetatileLabel = primaryTileset ? primaryTileset->metatileLabels.value(metatileId) : "";
    QString secondaryMetatileLabel = secondaryTileset ? secondaryTileset->metatileLabels.value(metatileId) : "";

    if (metatileId < Project::getNumMetatilesPrimary()) {
        labels.owned = primaryMetatileLabel;
        labels.shared = secondaryMetatileLabel;
    } else if (metatileId < Project::getNumMetatilesTotal()) {
        labels.owned = secondaryMetatileLabel;
        labels.shared = primaryMetatileLabel;
    }
    return labels;
}

// If the metatile has a label in the tileset it belongs to, return that label.
// If it doesn't, and the metatile has a label in the other tileset, return that label.
// Otherwise return an empty string.
QString Tileset::getMetatileLabel(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    MetatileLabelPair labels = Tileset::getMetatileLabelPair(metatileId, primaryTileset, secondaryTileset);
    return !labels.owned.isEmpty() ? labels.owned : labels.shared;
}

// Just get the "owned" metatile label, i.e. the one for the tileset that the metatile belongs to.
QString Tileset::getOwnedMetatileLabel(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset) {
    MetatileLabelPair labels = Tileset::getMetatileLabelPair(metatileId, primaryTileset, secondaryTileset);
    return labels.owned;
}

bool Tileset::setMetatileLabel(int metatileId, QString label, Tileset *primaryTileset, Tileset *secondaryTileset) {
    Tileset *tileset = Tileset::getMetatileTileset(metatileId, primaryTileset, secondaryTileset);
    if (!tileset)
        return false;

    if (!label.isEmpty()) {
        IdentifierValidator validator;
        if (!validator.isValid(label))
            return false;
    }

    tileset->metatileLabels[metatileId] = label;
    return true;
}

QString Tileset::getMetatileLabelPrefix()
{
    return Tileset::getMetatileLabelPrefix(this->name);
}

QString Tileset::getMetatileLabelPrefix(const QString &name)
{
    // Default is "gTileset_Name" --> "METATILE_Name_"
    const QString labelPrefix = projectConfig.getIdentifier(ProjectIdentifier::define_metatile_label_prefix);
    return QString("%1%2_").arg(labelPrefix).arg(Tileset::stripPrefix(name));
}

bool Tileset::metatileIsValid(uint16_t metatileId, const Tileset *primaryTileset, const Tileset *secondaryTileset) {
    return (primaryTileset && primaryTileset->containsMetatileId(metatileId))
        || (secondaryTileset && secondaryTileset->containsMetatileId(metatileId));
}

QList<QList<QRgb>> Tileset::getBlockPalettes(const Tileset *primaryTileset, const Tileset *secondaryTileset, bool useTruePalettes) {
    QList<QList<QRgb>> palettes;

    QList<QList<QRgb>> primaryPalettes;
    if (primaryTileset) {
        primaryPalettes = useTruePalettes ? primaryTileset->palettes : primaryTileset->palettePreviews;
    }
    for (int i = 0; i < Project::getNumPalettesPrimary(); i++) {
        palettes.append(primaryPalettes.value(i));
    }

    QList<QList<QRgb>> secondaryPalettes;
    if (secondaryTileset) {
        secondaryPalettes = useTruePalettes ? secondaryTileset->palettes : secondaryTileset->palettePreviews;
    }
    for (int i = Project::getNumPalettesPrimary(); i < Project::getNumPalettesTotal(); i++) {
        palettes.append(secondaryPalettes.value(i));
    }

    return palettes;
}

QList<QRgb> Tileset::getPalette(int paletteId, const Tileset *primaryTileset, const Tileset *secondaryTileset, bool useTruePalettes) {
    QList<QRgb> paletteTable;
    const Tileset *tileset = paletteId < Project::getNumPalettesPrimary()
            ? primaryTileset
            : secondaryTileset;
    if (!tileset) {
        return paletteTable;
    }

    auto palettes = useTruePalettes ? tileset->palettes : tileset->palettePreviews;
    if (paletteId < 0 || paletteId >= palettes.length()) {
        return paletteTable;
    }

    for (int i = 0; i < palettes.at(paletteId).length(); i++) {
        paletteTable.append(palettes.at(paletteId).at(i));
    }
    return paletteTable;
}

bool Tileset::appendToHeaders(const QString &filepath, const QString &friendlyName, bool usingAsm) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logError(QString("Could not write to file \"%1\"").arg(filepath));
        return false;
    }
    QString isSecondaryStr = this->is_secondary ? "TRUE" : "FALSE";
    QString dataString = "\n";
    if (usingAsm) {
        // Append to asm file
        dataString.append("\t.align 2\n");
        dataString.append(QString("%1::\n").arg(this->name));
        dataString.append("\t.byte TRUE @ is compressed\n");
        dataString.append(QString("\t.byte %1 @ is secondary\n").arg(isSecondaryStr));
        dataString.append("\t.2byte 0 @ padding\n");
        dataString.append(QString("\t.4byte gTilesetTiles_%1\n").arg(friendlyName));
        dataString.append(QString("\t.4byte gTilesetPalettes_%1\n").arg(friendlyName));
        dataString.append(QString("\t.4byte gMetatiles_%1\n").arg(friendlyName));
        if (projectConfig.baseGameVersion == BaseGameVersion::pokefirered) {
            dataString.append("\t.4byte NULL @ animation callback\n");
            dataString.append(QString("\t.4byte gMetatileAttributes_%1\n").arg(friendlyName));
        } else {
            dataString.append(QString("\t.4byte gMetatileAttributes_%1\n").arg(friendlyName));
            dataString.append("\t.4byte NULL @ animation callback\n");
        }
    } else {
        // Append to C file
        dataString.append(QString("const struct Tileset %1 =\n{\n").arg(this->name));
        if (projectConfig.tilesetsHaveIsCompressed) dataString.append("    .isCompressed = TRUE,\n");
        dataString.append(QString("    .isSecondary = %1,\n").arg(isSecondaryStr));
        dataString.append(QString("    .tiles = gTilesetTiles_%1,\n").arg(friendlyName));
        dataString.append(QString("    .palettes = gTilesetPalettes_%1,\n").arg(friendlyName));
        dataString.append(QString("    .metatiles = gMetatiles_%1,\n").arg(friendlyName));
        dataString.append(QString("    .metatileAttributes = gMetatileAttributes_%1,\n").arg(friendlyName));
        if (projectConfig.tilesetsHaveCallback) dataString.append("    .callback = NULL,\n");
        dataString.append("};\n");
    }
    file.write(dataString.toUtf8());
    file.flush();
    file.close();
    return true;
}

bool Tileset::appendToGraphics(const QString &filepath, const QString &friendlyName, bool usingAsm) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logError(QString("Could not write to file \"%1\"").arg(filepath));
        return false;
    }

    const QString tilesetDir = this->getExpectedDir();
    const QString tilesPath = QString("%1/tiles%2").arg(tilesetDir).arg(projectConfig.getIdentifier(ProjectIdentifier::tiles_output_extension));
    const QString palettesPath = tilesetDir + "/palettes/";
    const QString palettesExt = projectConfig.getIdentifier(ProjectIdentifier::pals_output_extension);

    QString dataString = "\n";
    if (usingAsm) {
        // Append to asm file
        dataString.append("\t.align 2\n");
        dataString.append(QString("gTilesetPalettes_%1::\n").arg(friendlyName));
        for (int i = 0; i < Project::getNumPalettesTotal(); i++)
            dataString.append(QString("\t.incbin \"%1%2%3\"\n").arg(palettesPath).arg(i, 2, 10, QLatin1Char('0')).arg(palettesExt));
        dataString.append("\n\t.align 2\n");
        dataString.append(QString("gTilesetTiles_%1::\n").arg(friendlyName));
        dataString.append(QString("\t.incbin \"%1\"\n").arg(tilesPath));
    } else {
        // Append to C file
        dataString.append(QString("const u16 gTilesetPalettes_%1[][%2] =\n{\n").arg(friendlyName).arg(Tileset::numColorsPerPalette()));
        for (int i = 0; i < Project::getNumPalettesTotal(); i++)
            dataString.append(QString("    INCBIN_U16(\"%1%2%3\"),\n").arg(palettesPath).arg(i, 2, 10, QLatin1Char('0')).arg(palettesExt));
        dataString.append("};\n");
        dataString.append(QString("\nconst u32 gTilesetTiles_%1[] = INCBIN_U32(\"%2\");\n").arg(friendlyName, tilesPath));
    }
    file.write(dataString.toUtf8());
    file.flush();
    file.close();
    return true;
}

bool Tileset::appendToMetatiles(const QString &filepath, const QString &friendlyName, bool usingAsm) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        logError(QString("Could not write to file \"%1\"").arg(filepath));
        return false;
    }

    const QString tilesetDir = this->getExpectedDir();
    // TODO(@traeighsea): update to determine which extension to use
    const QString metatilesPath = tilesetDir + "/metatiles.json";
    const QString metatileAttrsPath = tilesetDir + "/metatile_attributes.json";

    QString dataString = "\n";
    if (usingAsm) {
        // Append to asm file
        dataString.append("\t.align 1\n");
        dataString.append(QString("gMetatiles_%1::\n").arg(friendlyName));
        dataString.append(QString("\t.incbin \"%1\"\n").arg(metatilesPath));
        dataString.append(QString("\n\t.align 1\n"));
        dataString.append(QString("gMetatileAttributes_%1::\n").arg(friendlyName));
        dataString.append(QString("\t.incbin \"%1\"\n").arg(metatileAttrsPath));
    } else {
        // Append to C file
        dataString.append(QString("const u16 gMetatiles_%1[] = INCBIN_U16(\"%2\");\n").arg(friendlyName, metatilesPath));
        QString numBits = QString::number(projectConfig.metatileAttributesSize * 8);
        dataString.append(QString("const u%1 gMetatileAttributes_%2[] = INCBIN_U%1(\"%3\");\n").arg(numBits, friendlyName, metatileAttrsPath));
    }
    file.write(dataString.toUtf8());
    file.flush();
    file.close();
    return true;
}

// The path where Porymap expects a Tileset's graphics assets to be stored (but not necessarily where they actually are)
// Example: for gTileset_DepartmentStore, returns "data/tilesets/secondary/department_store"
QString Tileset::getExpectedDir()
{
    return Tileset::getExpectedDir(this->name, this->is_secondary);
}

QString Tileset::getExpectedDir(QString tilesetName, bool isSecondary)
{
    const QString basePath = isSecondary ? projectConfig.getFilePath(ProjectFilePath::data_secondary_tilesets_folders)
                                         : projectConfig.getFilePath(ProjectFilePath::data_primary_tilesets_folders);

    static const QRegularExpression re("([a-z])([A-Z0-9])");
    return basePath + Tileset::stripPrefix(tilesetName).replace(re, "\\1_\\2").toLower();
}

// Get the expected positions of the members in struct Tileset.
// Used when parsing asm tileset data, or C tileset data that's missing initializers.
QHash<int, QString> Tileset::getHeaderMemberMap(bool usingAsm)
{
     // The asm header has a padding field that needs to be skipped
    int paddingOffset = usingAsm ? 1 : 0;

    // The position of metatileAttributes changes between games
    bool isPokefirered = (projectConfig.baseGameVersion == BaseGameVersion::pokefirered);
    int metatileAttrPosition = (isPokefirered ? 6 : 5) + paddingOffset;

    auto map = QHash<int, QString>();
    map.insert(1, "isSecondary");
    map.insert(2 + paddingOffset, "tiles");
    map.insert(3 + paddingOffset, "palettes");
    map.insert(4 + paddingOffset, "metatiles");
    map.insert(metatileAttrPosition, "metatileAttributes");
    return map;
}

bool Tileset::loadMetatiles() {
    clearMetatiles();

    if (Util::hasExtension(this->metatiles_path, "json")) {
        return deserializeMetatilesFromJson();
    } else {
        return deserializeMetatilesFromBin();
    }
}

bool Tileset::saveMetatiles() {
    if (Util::hasExtension(this->metatiles_path, "json")) {
        return serializeMetatilesToJson();
    } else {
        return serializeMetatilesToBin();
    }
}

bool Tileset::loadMetatileAttributes() {
    if (Util::hasExtension(this->metatile_attrs_path, "json")) {
        return deserializeMetatileAttributesFromJson();
    } else {
       return deserializeMetatileAttributesFromBin();
    }
}

bool Tileset::saveMetatileAttributes() {
    if (Util::hasExtension(this->metatile_attrs_path, "json")) {
        return serializeMetatileAttributesToBin();
    } else {
        return serializeMetatileAttributesToJson();
    }
}

bool Tileset::loadTilesImage(QImage *importedImage) {
    QImage image;
    bool imported = false;
    if (importedImage) {
        image = *importedImage;
        imported = true;
    } else if (QFile::exists(this->tilesImagePath)) {
        // No image provided, load from file path.
        image = QImage(this->tilesImagePath).convertToFormat(QImage::Format_Indexed8, Qt::ThresholdDither);
    }

    if (image.isNull()) {
        logWarn(QString("Failed to load tiles image for %1. Using default tiles image.").arg(this->name));
        image = QImage(Tile::pixelWidth(), Tile::pixelHeight(), QImage::Format_Indexed8);
        image.fill(0);
    }

    // Validate image dimensions
    if (image.width() % Tile::pixelWidth() || image.height() % Tile::pixelHeight()) {
        logError(QString("%1 tiles image has invalid dimensions %2x%3. Dimensions must be a multiple of %4x%5.")
                            .arg(this->name)
                            .arg(image.width())
                            .arg(image.height())
                            .arg(Tile::pixelWidth())
                            .arg(Tile::pixelHeight()));
        return false;
    }

    // Validate the number of colors in the image.
    int colorCount = image.colorCount();
    if (colorCount > Tileset::numColorsPerPalette()) {
        flattenTo4bppImage(&image);
    } else if (colorCount < Tileset::numColorsPerPalette()) {
        QVector<QRgb> colorTable = image.colorTable();
        for (int i = colorTable.length(); i < Tileset::numColorsPerPalette(); i++) {
            colorTable.append(0);
        }
        image.setColorTable(colorTable);
    }
    m_tilesImage = image;

    // Cut up the full tiles image into individual tile images.
    m_tiles.clear();
    for (int y = 0; y < image.height(); y += Tile::pixelHeight())
    for (int x = 0; x < image.width(); x += Tile::pixelWidth()) {
        m_tiles.append(image.copy(x, y, Tile::pixelWidth(), Tile::pixelHeight()));
    }

    if (m_tiles.length() > maxTiles()) {
        logWarn(QString("%1 tile count of %2 exceeds limit of %3. Additional tiles will not be displayed.")
                            .arg(this->name)
                            .arg(m_tiles.length())
                            .arg(maxTiles()));

        // Just resize m_tiles so that numTiles() reports the correct tile count.
        // We'll leave m_tilesImage alone (it doesn't get displayed, and we don't want to delete the user's image data).
        m_tiles = m_tiles.mid(0, maxTiles());
    }

    if (imported) {
        // Only set this flag once we've successfully loaded the tiles image.
        m_hasUnsavedTilesImage = true;
    }

    return true;
}

bool Tileset::saveTilesImage() {
    // Only write the tiles image if it was changed.
    // Porymap will only ever change an existing tiles image by importing a new one.
    if (!m_hasUnsavedTilesImage)
        return true;

    if (!m_tilesImage.save(this->tilesImagePath, "PNG")) {
        logError(QString("Failed to save tiles image '%1'").arg(this->tilesImagePath));
        return false;
    }

    m_hasUnsavedTilesImage = false;
    return true;
}

bool Tileset::loadPalettes() {
    this->palettes.clear();
    this->palettePreviews.clear();

    for (int i = 0; i < Project::getNumPalettesTotal(); i++) {
        QList<QRgb> palette;
        QString path = this->palettePaths.value(i);
        if (!path.isEmpty()) {
            bool error = false;
            palette = PaletteUtil::parse(path, &error);
            if (error) palette.clear();
        }
        if (palette.isEmpty()) {
            // Either the palette failed to load, or no palette exists.
            // We expect tilesets to have a certain number of palettes,
            // so fill this palette with dummy colors.
            for (int j = 0; j < Tileset::numColorsPerPalette(); j++) {
                int colorComponent = j * (256 / Tileset::numColorsPerPalette());
                palette.append(qRgb(colorComponent, colorComponent, colorComponent));
            }
        }
        this->palettes.append(palette);
        this->palettePreviews.append(palette);
    }
    return true;
}

bool Tileset::savePalettes() {
    bool success = true;
    int numPalettes = qMin(this->palettePaths.length(), this->palettes.length());
    for (int i = 0; i < numPalettes; i++) {
        if (!PaletteUtil::writeJASC(this->palettePaths.at(i), this->palettes.at(i).toVector(), 0, Tileset::numColorsPerPalette()))
            success = false;
    }
    return success;
}

bool Tileset::load() {
    bool success = true;
    if (!loadPalettes()) success = false;
    if (!loadTilesImage()) success = false;
    if (!loadMetatiles()) success = false;
    if (!loadMetatileAttributes()) success = false;
    return success;
}

// Because metatile labels are global (and handled by the project) we don't save them here.
bool Tileset::save() {
    bool success = true;
    if (!savePalettes()) success = false;
    if (!saveTilesImage()) success = false;
    if (!saveMetatiles()) success = false;
    if (!saveMetatileAttributes()) success = false;
    return success;
}

QString Tileset::stripPrefix(const QString &fullName) {
    return QString(fullName).replace(projectConfig.getIdentifier(ProjectIdentifier::symbol_tilesets_prefix), "");
}

// Find which of the specified color IDs in 'searchColors' are not used by any of this tileset's metatiles.
// The 'pairedTileset' may be used to get the tile images for any tiles that don't belong to this tileset.
// If 'searchColors' is empty, it will for search for all unused colors.
QSet<int> Tileset::getUnusedColorIds(int paletteId, const Tileset *pairedTileset, const QSet<int> &searchColors) const {
    QSet<int> unusedColors = searchColors;
    if (unusedColors.isEmpty()) {
        // Search for all colors
        for (int i = 0; i < Tileset::numColorsPerPalette(); i++) {
            unusedColors.insert(i);
        }
    }
    const Tileset *primaryTileset = this->is_secondary ? pairedTileset : this;
    const Tileset *secondaryTileset = this->is_secondary ? this : pairedTileset;
    QSet<uint16_t> seenTileIds;
    for (const auto &metatile : m_metatiles)
    for (const auto &tile : metatile->tiles) {
        if (tile.palette != paletteId)
            continue;

        // Save time by ignoring tiles we've already inspected.
        if (seenTileIds.contains(tile.tileId))
            continue;
        seenTileIds.insert(tile.tileId);

        QImage image = getTileImage(tile.tileId, primaryTileset, secondaryTileset);
        if (image.isNull() || image.sizeInBytes() < Tile::numPixels())
            continue;

        const uchar * pixels = image.constBits();
        for (int i = 0; i < Tile::numPixels(); i++) {
            auto it = unusedColors.constFind(pixels[i]);
            if (it != unusedColors.constEnd()) {
                unusedColors.erase(it);
                if (unusedColors.isEmpty()) {
                    return {};
                }
            }
        }
    }
    return unusedColors;
}

// Returns the list of metatile IDs representing all the metatiles in this tileset that use the specified color ID.
QList<uint16_t> Tileset::findMetatilesUsingColor(int paletteId, int colorId, const Tileset *pairedTileset) const {
    const Tileset *primaryTileset = this->is_secondary ? pairedTileset : this;
    const Tileset *secondaryTileset = this->is_secondary ? this : pairedTileset;
    QSet<uint16_t> metatileIdSet;
    QHash<uint16_t, bool> tileContainsColor;
    uint16_t metatileIdBase = firstMetatileId();
    for (int i = 0; i < m_metatiles.length(); i++) {
        uint16_t metatileId = i + metatileIdBase;
        for (const auto &tile : m_metatiles.at(i)->tiles) {
            if (tile.palette != paletteId)
                continue;

            // Save time on tiles we've already inspected by getting the cached result.
            auto tileIt = tileContainsColor.constFind(tile.tileId);
            if (tileIt != tileContainsColor.constEnd()) {
                if (tileIt.value()) metatileIdSet.insert(metatileId);
                continue;
            }
            tileContainsColor[tile.tileId] = false;

            QImage image = getTileImage(tile.tileId, primaryTileset, secondaryTileset);
            if (image.isNull() || image.sizeInBytes() < Tile::numPixels())
                continue;

            const uchar * pixels = image.constBits();
            for (int j = 0; j < Tile::numPixels(); j++) {
                if (pixels[j] == colorId) {
                    metatileIdSet.insert(metatileId);
                    tileContainsColor[tile.tileId] = true;
                    break;
                }
            }
        }
    }
    QList<uint16_t> metatileIds(metatileIdSet.constBegin(), metatileIdSet.constEnd());
    std::sort(metatileIds.begin(), metatileIds.end());
    return metatileIds;
}

void Tileset::setNumPalettes(unsigned numPals) {
    m_numPals = numPals;
}

unsigned Tileset::getNumPalettes() const {
    return m_numPals.value_or(is_secondary ? Project::getNumPalettesTotal() - Project::getNumPalettesPrimary() : Project::getNumPalettesPrimary());
}

void Tileset::setMetatileAttrBitMasks(const QMap<QString, uint32_t> attrMasks) {
    m_metatileAttrBitMasks = attrMasks;
}

std::optional<QMap<QString, uint32_t>> Tileset::getMetatileAttrBitMasks() const {
    return m_metatileAttrBitMasks;
}

bool Tileset::deserializeMetatilesFromBin() {
    QFile file(this->metatiles_path);
    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open '%1' for reading: %2").arg(this->metatiles_path).arg(file.errorString()));
        return false;
    }

    QByteArray data = file.readAll();
    int tilesPerMetatile = projectConfig.getNumTilesInMetatile();
    int bytesPerMetatile = Tile::sizeInBytes() * tilesPerMetatile;
    int numberMetatiles = data.length() / bytesPerMetatile;
    if (numberMetatiles > maxMetatiles()) {
        logWarn(QString("%1 metatile count %2 exceeds limit of %3. Additional metatiles will be ignored.")
                        .arg(this->name)
                        .arg(numberMetatiles)
                        .arg(maxMetatiles()));
        numberMetatiles = maxMetatiles();
    }

    for (int i = 0; i < numberMetatiles; i++) {
        auto metatile = new Metatile;
        int index = i * bytesPerMetatile;
        for (int j = 0; j < tilesPerMetatile; j++) {
            uint16_t tileRaw = static_cast<unsigned char>(data[index++]);
            tileRaw |= static_cast<unsigned char>(data[index++]) << 8;
            metatile->tiles.append(Tile(tileRaw));
        }
        m_metatiles.append(metatile);
    }
    return true;
}

bool Tileset::deserializeMetatilesFromJson() {
    if (!Util::hasExtension(this->metatiles_path, "json")) {
        logError(QString("Incorrect file format, expected json extension %1").arg(metatiles_path));
        return false;
    }

    QJsonDocument metatilesDoc;
    ParseUtil parser;
    if (!parser.tryParseJsonFile(&metatilesDoc, metatiles_path)) {
        logError(QString("Failed to read metatiles from %1").arg(metatiles_path));
        return false;
    }

    QJsonObject metatilesObj = metatilesDoc.object();
    QJsonArray metatilesArr = metatilesObj["metatiles"].toArray();
    if (metatilesArr.size() == 0) {
        logError(QString("'metatiles' array is missing from %1.").arg(metatiles_path));
        return false;
    }

    m_numMetatiles.reset();
    bool succeeded{false};
    auto numMetatiles = ParseUtil::jsonToInt(metatilesObj["numMetatiles"], &succeeded);
    if (succeeded) {
        m_numMetatiles = numMetatiles;
    } else {
        int numMetatilesDefault = is_secondary ? Project::getNumMetatilesTotal() - Project::getNumMetatilesPrimary() : Project::getNumMetatilesPrimary();
        if (numMetatiles != numMetatilesDefault){
            logWarn(QString("Num metatiles %1 different than expected in %2").arg(numMetatiles).arg(metatiles_path));
        }
    }

    m_numTiles.reset();
    auto numTiles = ParseUtil::jsonToInt(metatilesObj["numTiles"], &succeeded);
    if (succeeded) {
        m_numTiles = numTiles;
    } else {
        int numTilesDefault = is_secondary ? Project::getNumTilesTotal() - Project::getNumTilesPrimary() : Project::getNumTilesPrimary();
        if (numTiles != numTilesDefault){
            logWarn(QString("Num tiles %1 different than expected in %2").arg(numTiles).arg(metatiles_path));
        }
    }

    m_numPals.reset();
    auto numPals = ParseUtil::jsonToInt(metatilesObj["numPals"], &succeeded);
    if (succeeded) {
        m_numPals = numPals;
    } else {
        int numPalsDefault = is_secondary ? Project::getNumPalettesTotal() - Project::getNumPalettesPrimary() : Project::getNumPalettesPrimary();
        if (numPals != numPalsDefault){
            logWarn(QString("Num palettes %1 different than expected in %2").arg(numPals).arg(metatiles_path));
        }
    }

    for (int i = 0; i < metatilesArr.size(); i++) {
        QJsonObject tilesObj = metatilesArr[i].toObject();
        if (tilesObj.isEmpty())
            continue;

        int tilesPerMetatile = projectConfig.getNumTilesInMetatile();
        QJsonArray tilesArr = tilesObj["tiles"].toArray();
        if (tilesArr.size() == 0) {
            logError(QString("'tiles' array is missing from %1.").arg(metatiles_path));
            return false;
        } else if (tilesArr.size() != tilesPerMetatile) {
            logError(QString("'tiles' at index %1 array size %2 different than project config Number of Tiles in Metatile %3.").arg(i).arg(tilesArr.size()).arg(tilesPerMetatile));
            return false;
        }

        Metatile* metatile = new Metatile();
        for (int j = 0; j < tilesPerMetatile; j++) {
            QJsonObject tileObj = tilesArr[j].toObject();
            if (tilesObj.isEmpty())
                continue;

            bool succeeded{true};
            Tile tile;

            tile.tileId = static_cast<uint16_t>(ParseUtil::jsonToInt(tileObj["tileId"], &succeeded));
            if (!succeeded) {
                logError(QString("Missing 'tileId' value on layout %1 in %2").arg(i).arg(metatiles_path));
                return false;
            }

            tile.xflip = static_cast<uint16_t>(ParseUtil::jsonToInt(tileObj["xflip"], &succeeded));
            if (!succeeded) {
                logError(QString("Missing 'xflip' value on layout %1 in %2").arg(i).arg(metatiles_path));
                return false;
            }

            tile.yflip = static_cast<uint16_t>(ParseUtil::jsonToInt(tileObj["yflip"], &succeeded));
            if (!succeeded) {
                logError(QString("Missing 'yflip' value on layout %1 in %2").arg(i).arg(metatiles_path));
                return false;
            }

            tile.palette = static_cast<uint16_t>(ParseUtil::jsonToInt(tileObj["palette"], &succeeded));
            if (!succeeded) {
                logError(QString("Missing 'palette' value on layout %1 in %2").arg(i).arg(metatiles_path));
                return false;
            }

            metatile->tiles.append(tile);
        }
        m_metatiles.append(std::move(metatile));
    }
    return true;
}

bool Tileset::deserializeMetatileAttributesFromBin() {
    QFile file(this->metatile_attrs_path);
    if (!file.open(QIODevice::ReadOnly)) {
        logError(QString("Could not open '%1' for reading: %2").arg(this->metatile_attrs_path).arg(file.errorString()));
        return false;
    }

    QByteArray data = file.readAll();
    int attrSize = projectConfig.metatileAttributesSize;
    int numberMetatiles = m_metatiles.length();
    int numMetatileAttrs = data.length() / attrSize;
    if (numMetatileAttrs > numberMetatiles) {
        logWarn(QString("%1 metatile attributes count %2 exceeds metatile count of %3. Additional attributes will be ignored.")
                            .arg(this->name)
                            .arg(numMetatileAttrs)
                            .arg(numberMetatiles));
        numMetatileAttrs = numberMetatiles;
    } else if (numMetatileAttrs < numberMetatiles) {
        logWarn(QString("%1 metatile attributes count %2 is fewer than the metatile count of %3. Missing attributes will default to 0.")
                            .arg(this->name)
                            .arg(numMetatileAttrs)
                            .arg(numberMetatiles));
    }

    for (int i = 0; i < numMetatileAttrs; i++) {
        uint32_t attributes = 0;
        for (int j = 0; j < attrSize; j++)
            attributes |= static_cast<unsigned char>(data.at(i * attrSize + j)) << (8 * j);
        m_metatiles.at(i)->setAttributes(attributes);
    }
    return true;
}

bool Tileset::deserializeMetatileAttributesFromJson() {
    if (!Util::hasExtension(this->metatile_attrs_path, "json")) {
        logError(QString("Incorrect file format, expected json extension %1").arg(metatile_attrs_path));
        return false;
    }

    QJsonDocument metatileAttrDoc;
    ParseUtil parser;
    if (!parser.tryParseJsonFile(&metatileAttrDoc, metatile_attrs_path)) {
        logError(QString("Failed to read metatile attrs from %1").arg(metatile_attrs_path));
        return false;
    }
    QJsonObject metatilesObj = metatileAttrDoc.object();

    bool succeeded{false};
    auto numMetatiles = ParseUtil::jsonToInt(metatilesObj["numMetatiles"], &succeeded);
    if (succeeded && m_numMetatiles != numMetatiles) {
        logWarn(QString("Num metatiles %1 different than expected in %2").arg(numMetatiles).arg(metatile_attrs_path));
    }

    QJsonObject attributeMasksObj = metatilesObj["attributeMasks"].toObject();

    m_metatileAttrBitMasks = QMap<QString, uint32_t>();
    auto metatileAttrPacker = std::make_shared<QMap<QString, BitPacker>>();
    // Loop through all the attribute masks
    // Allow arbitrarily named keys and set them accordingly
    for (auto key: attributeMasksObj.keys()){
        auto str = attributeMasksObj[key].toString();
        uint32_t bitmask = Util::hexToInt<uint32_t>(str);
        m_metatileAttrBitMasks->insert(key, bitmask);
        metatileAttrPacker->insert(key, bitmask);
    }

    QJsonArray metatileAttrArr = metatilesObj["metatiles"].toArray();
    if (metatileAttrArr.size() == 0) {
        logError(QString("'metatiles' array is missing from %1.").arg(metatile_attrs_path));
        return false;
    }

    for (int i = 0; i < metatileAttrArr.size(); i++) {
        QJsonObject metatileAttrObj = metatileAttrArr[i].toObject();
        if (metatileAttrObj.isEmpty()) {
            logError(QString("Issue parsing metatile attribute index %1 from %2.").arg(i).arg(metatile_attrs_path));
            continue;
        }

        m_metatiles.at(i)->setCustomBitPacker(metatileAttrPacker);

        QJsonObject attrObj = metatileAttrObj["attributes"].toObject();

        // Loop through all the attributes
        // Allow arbitrarily named keys and set them accordingly
        for (auto key: attrObj.keys()){
            bool succeeded{true};
            auto attrVal = static_cast<uint32_t>(ParseUtil::jsonToInt(attrObj[key], &succeeded));
            if (!succeeded) {
                logError(QString("Issue parsing metatile attribute %1 index %2 from %3.").arg(key).arg(i).arg(metatile_attrs_path));
                continue;
            }

            // Special case unused because we wanna preserve the data
            if (key == "unused") {
                auto unusedVal = m_metatiles.at(i)->getAttribute(Metatile::Attr::Unused);
                m_metatiles.at(i)->setAttribute(key, attrVal | unusedVal);
            } else {
                m_metatiles.at(i)->setAttribute(key, attrVal);
            }
        }
    }
    return true;
}

bool Tileset::serializeMetatilesToBin() {
    QFile file(this->metatiles_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        logError(QString("Could not open '%1' for writing: %2").arg(this->metatiles_path).arg(file.errorString()));
        return false;
    }

    QByteArray data;
    int numTiles = projectConfig.getNumTilesInMetatile();
    for (const auto &metatile : m_metatiles) {
        for (int i = 0; i < numTiles; i++) {
            uint16_t tile = metatile->tiles.value(i).rawValue();
            data.append(static_cast<char>(tile));
            data.append(static_cast<char>(tile >> 8));
        }
    }
    file.write(data);
    return true;
}

bool Tileset::serializeMetatilesToJson() {
    QFile metatilesFile(metatiles_path);
    if (metatilesFile.open(QIODevice::WriteOnly)) {
        OrderedJson::object metatilesObj;

        int numberMetatiles = numMetatiles();
        metatilesObj["numMetatiles"] = numberMetatiles;

        int numberTiles = numTiles();
        metatilesObj["numTiles"] = numberTiles;

        int numPals = getNumPalettes();
        metatilesObj["numPals"] = numPals;

        int numTilesInMetatile = projectConfig.getNumTilesInMetatile();
        if (m_metatiles[0]->tiles.size() != numTilesInMetatile) {
            numTilesInMetatile = m_metatiles[0]->tiles.size();
        }
        metatilesObj["numTilesInMetatile"] = numTilesInMetatile;

        OrderedJson::array metatilesArr;
        for (Metatile *metatile : m_metatiles) {
            OrderedJson::array tileArr;
            for (int i = 0; i < metatile->tiles.size(); i++) {
                OrderedJson::object tileObj;
                tileObj["tileId"] = metatile->tiles[i].tileId;
                tileObj["xflip"] = metatile->tiles[i].xflip;
                tileObj["yflip"] = metatile->tiles[i].yflip;
                tileObj["palette"] = metatile->tiles[i].palette;
                tileArr.push_back(tileObj);
            }
            OrderedJson::object metatileObj;
            metatileObj["tiles"] = tileArr;

            metatilesArr.push_back(metatileObj);
        }
        metatilesObj["metatiles"] = metatilesArr;

        OrderedJson metatilesJson(metatilesObj);
        OrderedJsonDoc jsonDoc(&metatilesJson);
        jsonDoc.dump(&metatilesFile);
    } else {
        logError(QString("Could not open tileset metatile attr file '%1'").arg(metatiles_path));
        return false;
    }
    return true;
}

bool Tileset::serializeMetatileAttributesToBin() {
    QFile file(this->metatile_attrs_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        logError(QString("Could not open '%1' for writing: %2").arg(this->metatile_attrs_path).arg(file.errorString()));
        return false;
    }

    QByteArray data;
    for (const auto &metatile : m_metatiles) {
        uint32_t attributes = metatile->getAttributes();
        for (int i = 0; i < projectConfig.metatileAttributesSize; i++)
            data.append(static_cast<char>(attributes >> (8 * i)));
    }
    file.write(data);
    return true;
}

bool Tileset::serializeMetatileAttributesToJson() {
    QFile metatileAttrFile(metatile_attrs_path);
    if (metatileAttrFile.open(QIODevice::WriteOnly)) {      
        OrderedJson::object metatilesObj;

        int numberMetatiles = numMetatiles();
        metatilesObj["numMetatiles"] = numberMetatiles;

        int metatileAttributeSize = projectConfig.metatileAttributesSize * 8;
        metatilesObj["attributeSizeInBits"] = metatileAttributeSize;

        OrderedJson::object attributeMasksObj;
        if (m_metatileAttrBitMasks) {
            for (auto& key: m_metatileAttrBitMasks->keys()){
                attributeMasksObj[key] = Util::intToHex(m_metatileAttrBitMasks->value(key));
            }
        } else {
            uint32_t behaviorMask = projectConfig.metatileBehaviorMask;
            uint32_t terrainTypeMask = projectConfig.metatileTerrainTypeMask;
            uint32_t encounterTypeMask = projectConfig.metatileEncounterTypeMask;
            uint32_t layerTypeMask = projectConfig.metatileLayerTypeMask;
            // Calculate mask of bits not used by standard behaviors so we can preserve this data.
            uint32_t unusedMask = ~(behaviorMask | terrainTypeMask | encounterTypeMask | layerTypeMask);
            unusedMask &= Metatile::getMaxAttributesMask();

            attributeMasksObj[AttrConsts::BehaviorStr] = Util::intToHex(behaviorMask);
            attributeMasksObj[AttrConsts::TerrainTypeStr] = Util::intToHex(terrainTypeMask);
            attributeMasksObj[AttrConsts::EncounterTypeStr] = Util::intToHex(encounterTypeMask);
            attributeMasksObj[AttrConsts::LayerTypeStr] = Util::intToHex(layerTypeMask);
            attributeMasksObj[AttrConsts::UnusedStr] = Util::intToHex(unusedMask);
        }
        metatilesObj["attributeMasks"] = attributeMasksObj;

        OrderedJson::array metatileAttrArr;
        for (Metatile *metatile : m_metatiles) {
            OrderedJson::object metatileAttributeObj;
            auto keys = metatile->getAttributeKeys();
            for (auto attribute: keys) {
                metatileAttributeObj[attribute] = static_cast<int>(metatile->getAttribute(attribute));
            }
            OrderedJson::object metatileObj;
            metatileObj["attributes"] = metatileAttributeObj;

            metatileAttrArr.push_back(metatileObj);
        }
        metatilesObj["metatiles"] = metatileAttrArr;

        OrderedJson metatileAttrsJson(metatilesObj);
        OrderedJsonDoc jsonDoc(&metatileAttrsJson);
        jsonDoc.dump(&metatileAttrFile);
    } else {
        logError(QString("Could not open tileset metatiles file '%1'").arg(metatile_attrs_path));
        return false;
    }
    return true;
}

