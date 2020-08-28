#
# chatterbox Progetto del corso di LSO 2017
#
# Dipartimento di Informatica Università di Pisa
# Docenti: Prencipe, Torquati
#
# @author Monica Amico 516801
# Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
# originale dell'autore  
#


# Stampo l'uso dello script 
# se il numero dei parametri non è corretto
if [ $# -lt 2 ]; 
then
    echo "Utilizzare: $0 file.conf time" 1>&2
    exit 1
fi

# Se --help è presente stampo l'uso dello script
for parametro in "$@"
do  
    if [ "$parametro" == "--help" ];
    then
        echo "Utilizzare: $0 file.conf time" 1>&2
        exit 1
    fi
done

# Controllo sse il file passato come parametro esiste
if ! [ -e $1 ];
then
    echo "Il file $1 è inesistente!" 1>&2
    exit 2
fi

# Prendo il contenuto del file che contiene
# il nome della cartella
DirName="$(grep DirName $1)"

#controllo se è presente il nome della cartella
if [ "$DirName" = "" ];
then 
    echo "Nessun DirName trovato" 1>&2
    exit 3
fi

DirName=${DirName##*"DirName"}
DirName=${DirName#*"="}
DirName=${DirName#*'\s'}
DirName=${DirName//[[:space:]]/}
echo "Cartella:$DirName" 1>&2

T=$2

# CASO: T > 0 
if  [ $T -gt 0 ];
then  
    echo "Files modificati da più di $T minuti:" 1>&2
    Files=$DirName/*
    Date=$(date +%s)

    for F in $Files
    do
        FileD=$(stat --format=%Y $F)
        Minutes=$(( ($Date-$FileD)/60)) 
        # Se i minuti sono maggiori di T
        if [ $Minutes -gt $T ];
        then
            echo "rimuovo $F nessuna modifica da $Minutes minuti" 1>&2
            rm -R $F
        fi
    done

# CASO: T = 0
elif [ $T -eq 0 ];
then
    echo "Lista dei files" 1>&2

    # lista di tutti i files nella cartella
    Files=$DirName/*

    for F in $Files
    do 
        # controllo se è un file regolare
         Regular=$(stat --format=%F $F)
         if [ "$Regular" = "file regolare" ];
         then               
             File=${F##*/}
             echo "> $File" 1>&2
         fi        
    done
fi

exit 0