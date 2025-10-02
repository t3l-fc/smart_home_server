# Configuration ESP32 → Render Auto-Redeploy

## 🎯 Ce que ça fait

Votre ESP32 va maintenant :
- **Surveiller** votre serveur Render toutes les 10 minutes
- **Détecter** quand il ne répond plus
- **Redéployer automatiquement** le dernier commit
- **Afficher le statut** sur l'écran 7-segments

## 📋 Étapes de configuration

### 1. Obtenir votre Deploy Hook URL

1. Allez sur [render.com](https://render.com) et connectez-vous
2. Sélectionnez votre service `smart-home-server-a076`
3. Cliquez sur l'onglet **"Settings"**
4. Cherchez la section **"Deploy Hook"** ou **"Build & Deploy"**
5. Cliquez sur **"Create Deploy Hook"** si pas encore fait
6. Copiez l'URL qui ressemble à :
   ```
   https://api.render.com/deploy/srv-xxxxxxxxxxxxx?key=yyyyyyyyyyyy
   ```

### 2. Configurer l'ESP32

Dans le fichier `arduino/src/main.cpp`, ligne 68, remplacez :

```cpp
renderManager.setDeployHookUrl("https://api.render.com/deploy/srv-YOUR_SERVICE_ID?key=YOUR_KEY");
```

Par votre vraie URL Deploy Hook.

### 3. Compiler et uploader

```bash
cd arduino
pio run --target upload
```

## 📊 Affichage sur l'écran 7-segments

Votre écran affichera maintenant 5 caractères :
- **Positions 1-4** : États des prises (0/1)
- **Position 5** : Statut serveur
  - `H` = Healthy (serveur OK)
  - `C` = Checking (vérification en cours)
  - `E` = Error (serveur en panne)
  - `D` = Deploying (redéploiement en cours)
  - `-` = Unknown (statut inconnu)

## 🔧 Paramètres configurables

Dans le setup(), vous pouvez ajuster :

```cpp
renderManager.setCheckInterval(10 * 60 * 1000); // Intervalle de vérification (10 min)
renderManager.setMaxFailures(2);                // Redéploie après 2 échecs
```

## 📝 Monitoring via Serial

Ouvrez le Serial Monitor (115200 baud) pour voir :
- Statut des vérifications serveur
- Déclenchement des redéploiements
- Messages d'erreur éventuels

## 🚀 Test du système

1. **Test manuel** : Éteignez votre serveur Render depuis le dashboard
2. **Attendez** : L'ESP32 va détecter l'échec après max 10 minutes
3. **Vérifiez** : Le redéploiement automatique devrait se déclencher
4. **Observez** : Le statut sur l'écran change de `H` → `C` → `E` → `D` → `H`

## ⚡ Avantages vs UptimeRobot

- ✅ **Contrôle total** : Pas de dépendance externe
- ✅ **Logique intelligente** : Réessaie avant de redéployer
- ✅ **Intégration native** : Affiché sur votre écran
- ✅ **Pas de limite** : Pas de restrictions comme UptimeRobot gratuit
- ✅ **Réactivité** : Redéploiement immédiat quand détecté

## 🔍 Dépannage

Si ça ne marche pas :

1. **Vérifiez l'URL Deploy Hook** dans le Serial Monitor
2. **Testez manuellement** l'URL avec curl :
   ```bash
   curl -X POST "VOTRE_DEPLOY_HOOK_URL"
   ```
3. **Vérifiez WiFi** : L'ESP32 doit être connecté
4. **Regardez les logs Render** pour voir si le redéploiement se déclenche

## 📞 Support

Si vous avez des questions, regardez les messages dans le Serial Monitor - ils sont très détaillés !
