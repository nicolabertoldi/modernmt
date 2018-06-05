package eu.modernmt.decoder;

import eu.modernmt.lang.LanguagePair;
import eu.modernmt.model.ContextVector;
import eu.modernmt.model.Sentence;
import eu.modernmt.model.Translation;

/**
 * Created by davide on 23/05/17.
 */
public interface DecoderWithNBest {

    Translation translate(long user, LanguagePair direction, Sentence text, int nbestListSize) throws DecoderException;

    Translation translate(long user, LanguagePair direction, Sentence text, ContextVector contextVector, int nbestListSize) throws DecoderException;

}
