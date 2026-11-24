import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

df = pd.read_csv('./hk-time.csv')
df['time'] /= 1_000_000_000

fig, ax = plt.subplots(figsize=(8, 5), dpi=200)

sns.lineplot(
  data=df,
  x='nv',
  y='time',
  hue='family',
  marker='o',
  errorbar=('sd', 1),
  ax=ax,
)

ax.set_xlabel('Vertices')
ax.set_ylabel('Runtime')
ax.yaxis.set_major_formatter(mticker.EngFormatter(unit='s'))

fig.tight_layout(pad=0.5)
fig.savefig('hk-time.png', dpi=200, bbox_inches='tight', pad_inches=0.1)
