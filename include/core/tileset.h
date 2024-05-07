#pragma once
#ifndef TILESET_H
#define TILESET_H

#include "metatile.h"
#include "tile.h"
#include <QImage>
#include <QHash>

struct MetatileLabelPair {
    QString owned;
    QString shared;
};

class Tileset
{
public:
    Tileset() = default;
    Tileset(const Tileset &other);
    Tileset &operator=(const Tileset &other);

public:
    QString name;
    bool is_secondary;
    QString tiles_label;
    QString palettes_label;
    QString metatiles_label;
    QString metatiles_path;
    QString metatile_attrs_label;
    QString metatile_attrs_path;
    QString tilesImagePath;
    QImage tilesImage;
    QStringList palettePaths;

    QList<QImage> tiles;
    QList<Metatile*> metatiles;
    QHash<int, QString> metatileLabels;
    QList<QList<QRgb>> palettes;
    QList<QList<QRgb>> palettePreviews;

    bool hasUnsavedTilesImage;

    /// Used in conjunction when enabling json with metatiles because it can allow us to know how to
    /// pack and unpack the binary in the case of multiple metatile packing formats in a single project.
    /// When using this member, it will ignore the project settings for how to pack attr bits
    std::optional<QMap<QString, uint32_t>> metatileAttrBitMasks{};
    /// Used in conjunction when enabling json with metatiles which can allow for arbitrary sizes
    std::optional<unsigned> numMetatiles{};
    /// Used in conjunction when enabling json with metatiles which can allow for arbitrary sizes
    std::optional<unsigned> numTiles{};
    /// Used in conjunction when enabling json with metatiles which can allow for arbitrary sizes
    std::optional<unsigned> numPals{};

    static Tileset* getMetatileTileset(int, Tileset*, Tileset*);
    static Tileset* getTileTileset(int, Tileset*, Tileset*);
    static Metatile* getMetatile(int, Tileset*, Tileset*);
    static Tileset* getMetatileLabelTileset(int, Tileset*, Tileset*);
    static QString getMetatileLabel(int, Tileset *, Tileset *);
    static QString getOwnedMetatileLabel(int, Tileset *, Tileset *);
    static MetatileLabelPair getMetatileLabelPair(int metatileId, Tileset *primaryTileset, Tileset *secondaryTileset);
    static bool setMetatileLabel(int, QString, Tileset *, Tileset *);
    QString getMetatileLabelPrefix();
    static QString getMetatileLabelPrefix(const QString &name);
    static QList<QList<QRgb>> getBlockPalettes(Tileset*, Tileset*, bool useTruePalettes = false);
    static QList<QRgb> getPalette(int, Tileset*, Tileset*, bool useTruePalettes = false);
    static bool metatileIsValid(uint16_t metatileId, Tileset *, Tileset *);
    static QHash<int, QString> getHeaderMemberMap(bool usingAsm);
    static QString getExpectedDir(QString tilesetName, bool isSecondary);
    QString getExpectedDir();
    bool appendToHeaders(QString root, QString friendlyName, bool usingAsm);
    bool appendToGraphics(QString root, QString friendlyName, bool usingAsm);
    bool appendToMetatiles(QString root, QString friendlyName, bool usingAsm);

    int getNumMetatiles() const;
    int getNumTiles() const;
    int getNumPalettes() const;
};

#endif // TILESET_H
