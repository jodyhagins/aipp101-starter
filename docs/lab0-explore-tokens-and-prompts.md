# Lab 0: Explore Tokens & Prompts

In this lab you will use three interactive web tools to build intuition for
how large language models (LLMs) work under the hood. No code to write -- just
experiment, observe, and reflect.

By the end you should be able to answer:

- What is a token, and why does tokenization matter?
- How does the same text get tokenized differently by different models?
- How does a Transformer predict the next token?
- What role do temperature, top-k, and top-p play in generation?

---

## Part 1 -- Tokenization with Tiktokenizer

**URL:** <https://tiktokenizer.vercel.app>

Tiktokenizer lets you type (or paste) text and instantly see how a model's
tokenizer breaks it into tokens. Each token is color-coded so you can see
exactly where the boundaries fall.

### Getting oriented

1. Open the site. You should see a text input area and a model selector
   (defaults to **GPT-4o**).
2. Type a short English sentence, e.g. `The quick brown fox jumps over the
   lazy dog.` Note the **token count** displayed.
3. Toggle **Show whitespace** to see how spaces and newlines become part of
   tokens.

### Exercises

#### 1a -- English text
Type or paste several English sentences. Observe:

- Where does the tokenizer split words?
- Are common words a single token? What about rare or long words?
- How are punctuation marks handled?

#### 1b -- C++ code
Paste the following C++ snippet:

```cpp
std::vector<std::pair<int,int>> v;
for (auto& [key, val] : my_map) {
    v.emplace_back(key, val);
}
```

- How many tokens does this produce?
- Notice how angle brackets, colons, and other syntax characters are tokenized.
- Try adding comments or longer variable names and watch the count change.

#### 1c -- Compare across models
Switch the model selector to a different model (e.g. **Llama 3**, **GPT-3.5**,
or **Claude**). Paste the *same* text and compare:

- Does the token count change?
- Do the token boundaries shift?
- Which model is more "efficient" (fewer tokens) for C++ code? For English
  prose?

#### 1d -- Adversarial inputs
Try some unusual inputs and see what happens:

- Emojis: `Hello 👋🌍`
- A URL: `https://en.wikipedia.org/wiki/Transformer_(deep_learning_architecture)`
- Repeated characters: `aaaaaaaaaaaaaaaaaaaaa`
- Mixed languages: `Hello 世界 مرحبا мир`
- Numbers: `3.14159265358979` vs `three point one four`

> **Takeaway:** Tokenization is *not* the same as splitting on spaces. Different
> models use different vocabularies, which means the same prompt can cost more
> or fewer tokens depending on the model.

---

## Part 2 -- Transformer Explainer

**URL:** <https://poloclub.github.io/transformer-explainer>

Transformer Explainer is an interactive visualization of the GPT-2 (small)
architecture -- 124 million parameters running *directly in your browser* via
ONNX Runtime. It lets you see every stage of how a Transformer processes
input and predicts the next token.

### Getting oriented

1. Open the site. You will see a diagram of the full Transformer pipeline:
   **Input** -> **Embedding** -> **Transformer Blocks** -> **Output
   Probabilities**.
2. There is a text input box at the top where you can type a prompt.
3. On the right side you will see **sampling controls**: Temperature, Top-k,
   and Top-p sliders.

### Exercises

#### 2a -- Watch a prediction happen
1. Type a partial sentence, e.g. `The capital of France is`
2. Observe the output probabilities on the right. What token has the highest
   probability? Does the model get it right?
3. Try other factual completions:
   - `The color of the sky is`
   - `Water freezes at`
   - `One plus one equals`

#### 2b -- Explore the embedding layer
Click on the **Embedding** section of the diagram to expand it. Observe:

- **Tokenization:** The input text is split into tokens (just like Part 1).
- **Token embeddings:** Each token is mapped to a 768-dimensional vector.
- **Positional encoding:** Position information is added so the model knows
  word order.
- The token embedding and positional encoding are *summed* to produce the
  final input to the Transformer blocks.

#### 2c -- Explore attention
Click on a **Transformer Block** to expand it. Inside you will find
**Multi-Head Self-Attention**:

- Hover over individual tokens to see their **attention weights** -- which
  other tokens does the model "pay attention to" when processing this token?
- Notice how different attention heads focus on different relationships
  (some focus on nearby words, others on syntactic or semantic connections).
- The model has **12 attention heads** per block. Try to spot different
  patterns across heads.

#### 2d -- Experiment with temperature
Use the **Temperature** slider:

- **Temperature = 0** (or very low): The model is highly confident and almost
  always picks the top token. Outputs are deterministic and repetitive.
- **Temperature = 1**: Standard randomness. The probability distribution is
  used as-is.
- **Temperature > 1**: The distribution flattens out. Less likely tokens become
  more probable. Outputs get more creative (and more chaotic).

Try generating from the same prompt at different temperatures. How does the
output change?

#### 2e -- Experiment with Top-k and Top-p
- **Top-k**: Only the top *k* most probable tokens are considered. Set k=1 and
  you get greedy decoding. Set k=50 and the model can choose from a wider pool.
- **Top-p** (nucleus sampling): Instead of a fixed count, include tokens until
  their cumulative probability reaches *p*. For example, top-p=0.9 means the
  model considers the smallest set of tokens whose probabilities sum to 90%.

Try different combinations and notice how they affect diversity and coherence.

> **Takeaway:** A Transformer converts tokens into embeddings, processes them
> through layers of self-attention and feedforward networks, and outputs a
> probability distribution over the entire vocabulary. Temperature, top-k, and
> top-p control *how* the model samples from that distribution.

---

## Part 3 -- Next Token Prediction Visualization

**URL:** <https://alonsosilva-nexttokenprediction.hf.space>

This Hugging Face Space (built with Solara) runs GPT-2 and visualizes the
autoregressive generation process step by step. It may take a moment to load
on first visit.

### Getting oriented

1. Open the site. You will see a text input box.
2. Type a short prompt, e.g. `One, two,`
3. The app will display the **predicted next token** along with its
   **probability** (e.g. "three" at 39.71%).

### Exercises

#### 3a -- Observe greedy decoding
1. Enter `One, two,` and note the predicted next token and its probability.
2. The app uses **greedy sampling** (always picks the highest-probability
   token). Watch it generate a sequence step by step.
3. Try other predictable sequences:
   - `A, B, C,`
   - `Monday, Tuesday,`
   - `Once upon a`

How confident is the model for each prediction? Is the probability always
high, or does it vary?

#### 3b -- Observe uncertainty
Try prompts where the continuation is ambiguous:

- `I went to the`  (store? park? doctor? ...)
- `The best programming language is`
- `She said`

When there are many plausible continuations, the top probability should be
lower. This reflects the model's **uncertainty**.

#### 3c -- Token-level surprise
If available, check out the companion **Perplexity** app:
<https://alonsosilva-perplexity.hf.space>

This shows how *surprised* the model is by each token in a sentence.

- Paste a simple sentence like `The cat sat on the mat.`
- Common, predictable words (like `the`, `on`) should show low surprise.
- Unusual or unexpected words should show high surprise (high perplexity).
- Try a grammatically odd sentence and see how surprise spikes.

> **Takeaway:** LLMs generate text one token at a time by predicting the next
> token given everything before it. The probability distribution over the
> vocabulary tells us how confident the model is. Greedy decoding always picks
> the most likely token, but that is not always the best strategy for open-ended
> generation.

---

## Reflection Questions

After completing all three parts, consider:

1. **Why does tokenization matter for cost?** If model A tokenizes your prompt
   into 50 tokens and model B into 80 tokens, what are the implications for
   API pricing and context window usage?

2. **Why does tokenization matter for performance?** Could a model that
   tokenizes C++ poorly also *understand* C++ poorly? Why or why not?

3. **What is the relationship between temperature and creativity?** When would
   you want low temperature? High temperature?

4. **Why is greedy decoding not always best?** Think of a scenario where always
   picking the most probable next token leads to worse output than sampling.

5. **What surprised you most?** Which visualization gave you the best intuition
   for how LLMs actually work?
