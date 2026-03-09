#ifndef TYPE_H
#define TYPE_H

enum Type {
    INT,
    DOUBLE,
    VOID,
    ADDR  // type pointeur/adresse (8 octets, pour les lvalues)
};

// Retourne la taille en octets d'un type
inline int typeSize(Type t) {
    switch (t) {
        case DOUBLE: return 8;
        case ADDR:   return 8;
        case INT:    return 4;
        default:     return 4;
    }
}

// Retourne le type "dominant" pour les conversions implicites (int → double)
inline Type promoteType(Type a, Type b) {
    if (a == DOUBLE || b == DOUBLE) return DOUBLE;
    return INT;
}

#endif
